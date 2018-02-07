// Copyright 2016 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devhost.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <threads.h>
#include <unistd.h>

#include <ddk/device.h>
#include <ddk/driver.h>

#include <zircon/assert.h>
#include <zircon/listnode.h>
#include <zircon/syscalls.h>
#include <zircon/thread_annotations.h>
#include <zircon/types.h>

#include <fdio/remoteio.h>

#define TRACE 0

#if TRACE
#define xprintf(fmt...) printf(fmt)
#else
#define xprintf(fmt...) \
    do {                \
    } while (0)
#endif

#define TRACE_ADD_REMOVE 0

#define BOOT_FIRMWARE_DIR "/boot/lib/firmware"
#define SYSTEM_FIRMWARE_DIR "/system/lib/firmware"

bool __dm_locked = false;
mtx_t __devhost_api_lock = MTX_INIT;

static thread_local creation_context_t* creation_ctx;

// The creation context is setup before the bind() or create() ops are
// invoked to provide the ability to sanity check the required device_add()
// operations these hooks should be making.
void devhost_set_creation_context(creation_context_t* ctx) {
    creation_ctx = ctx;
}

static zx_status_t default_open(void* ctx, zx_device_t** out, uint32_t flags) {
    return ZX_OK;
}

static zx_status_t default_open_at(void* ctx, zx_device_t** out, const char* path, uint32_t flags) {
    return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t default_close(void* ctx, uint32_t flags) {
    return ZX_OK;
}

static void default_unbind(void* ctx) {
}

static void default_release(void* ctx) {
}

static zx_status_t default_read(void* ctx, void* buf, size_t count, zx_off_t off, size_t* actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t default_write(void* ctx, const void* buf, size_t count, zx_off_t off, size_t* actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

static zx_off_t default_get_size(void* ctx) {
    return 0;
}

static zx_status_t default_ioctl(void* ctx, uint32_t op,
                             const void* in_buf, size_t in_len,
                             void* out_buf, size_t out_len, size_t* out_actual) {
    return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t default_suspend(void* ctx, uint32_t flags) {
    return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t default_resume(void* ctx, uint32_t flags) {
    return ZX_ERR_NOT_SUPPORTED;
}

static zx_status_t default_rxrpc(void* ctx, zx_handle_t channel) {
    return ZX_ERR_NOT_SUPPORTED;
}

zx_protocol_device_t device_default_ops = {
    .open = default_open,
    .open_at = default_open_at,
    .close = default_close,
    .unbind = default_unbind,
    .release = default_release,
    .read = default_read,
    .write = default_write,
    .get_size = default_get_size,
    .ioctl = default_ioctl,
    .suspend = default_suspend,
    .resume = default_resume,
    .rxrpc = default_rxrpc,
};

static struct list_node unmatched_device_list = LIST_INITIAL_VALUE(unmatched_device_list);
static struct list_node driver_list = LIST_INITIAL_VALUE(driver_list);

void dev_ref_release(zx_device_t* dev) TA_REQ(&__devhost_api_lock) {
    if (dev->refcount < 1) {
        printf("device: %p: REFCOUNT GOING NEGATIVE\n", dev);
        //TODO: probably should assert, but to start with let's
        //      see if this is happening in normal use
    }
    dev->refcount--;
    if (dev->refcount == 0) {
        if (dev->flags & DEV_FLAG_INSTANCE) {
            // these don't get removed, so mark dead state here
            dev->flags |= DEV_FLAG_DEAD | DEV_FLAG_VERY_DEAD;
            list_delete(&dev->node);
        }
        if (dev->flags & DEV_FLAG_BUSY) {
            // this can happen if creation fails
            // the caller to device_add() will free it
            printf("device: %p(%s): ref=0, busy, not releasing\n", dev, dev->name);
            return;
        }
#if TRACE_ADD_REMOVE
        printf("device: %p(%s): ref=0. releasing.\n", dev, dev->name);
#endif

        if (!(dev->flags & DEV_FLAG_VERY_DEAD)) {
            printf("device: %p(%s): only mostly dead (this is bad)\n", dev, dev->name);
        }
        if (!list_is_empty(&dev->children)) {
            printf("device: %p(%s): still has children! not good.\n", dev, dev->name);
        }

        zx_handle_close(dev->event);
        zx_handle_close(dev->local_event);
        DM_UNLOCK();
        dev_op_release(dev);
        DM_LOCK();

        // At this point we can safely release the ref on our parent
        if (dev->parent) {
            dev_ref_release(dev->parent);
        }
    }
}

zx_status_t devhost_device_create(zx_driver_t* drv, zx_device_t* parent,
                                  const char* name, void* ctx,
                                  zx_protocol_device_t* ops, zx_device_t** out) {

    if (!drv) {
        printf("devhost: _device_add could not find driver!\n");
        return ZX_ERR_INVALID_ARGS;
    }

    zx_device_t* dev = malloc(sizeof(zx_device_t));
    if (dev == NULL) {
        return ZX_ERR_NO_MEMORY;
    }

    memset(dev, 0, sizeof(zx_device_t));
    dev->magic = DEV_MAGIC;
    dev->ops = ops;
    dev->driver = drv;
    list_initialize(&dev->children);
    list_initialize(&dev->instances);

    if (name == NULL) {
        printf("devhost: dev=%p has null name.\n", dev);
        name = "invalid";
        dev->magic = 0;
    }

    size_t len = strlen(name);
    if (len >= ZX_DEVICE_NAME_MAX) {
        printf("devhost: dev=%p name too large '%s'\n", dev, name);
        len = ZX_DEVICE_NAME_MAX - 1;
        dev->magic = 0;
    }

    memcpy(dev->name, name, len);
    dev->name[len] = 0;
    dev->ctx = ctx ? ctx : dev;
    *out = dev;
    return ZX_OK;
}

#define DEFAULT_IF_NULL(ops,method) \
    if (ops->method == NULL) { \
        ops->method = default_##method; \
    }

static zx_status_t device_validate(zx_device_t* dev) {
    if (dev == NULL) {
        printf("INVAL: NULL!\n");
        return ZX_ERR_INVALID_ARGS;
    }
    if (dev->flags & DEV_FLAG_ADDED) {
        printf("device already added: %p(%s)\n", dev, dev->name);
        return ZX_ERR_BAD_STATE;
    }
    if (dev->magic != DEV_MAGIC) {
        return ZX_ERR_BAD_STATE;
    }
    if (dev->ops == NULL) {
        printf("device add: %p(%s): NULL ops\n", dev, dev->name);
        return ZX_ERR_INVALID_ARGS;
    }
    if ((dev->protocol_id == ZX_PROTOCOL_MISC_PARENT) ||
        (dev->protocol_id == ZX_PROTOCOL_ROOT)) {
        // These protocols is only allowed for the special
        // singleton misc or root parent devices.
        return ZX_ERR_INVALID_ARGS;
    }
    // devices which do not declare a primary protocol
    // are implied to be misc devices
    if (dev->protocol_id == 0) {
        dev->protocol_id = ZX_PROTOCOL_MISC;
    }

    // install default methods if needed
    zx_protocol_device_t* ops = dev->ops;
    DEFAULT_IF_NULL(ops, open);
    DEFAULT_IF_NULL(ops, open_at);
    DEFAULT_IF_NULL(ops, close);
    DEFAULT_IF_NULL(ops, unbind);
    DEFAULT_IF_NULL(ops, release);
    DEFAULT_IF_NULL(ops, read);
    DEFAULT_IF_NULL(ops, write);
    DEFAULT_IF_NULL(ops, get_size);
    DEFAULT_IF_NULL(ops, ioctl);
    DEFAULT_IF_NULL(ops, suspend);
    DEFAULT_IF_NULL(ops, resume);
    DEFAULT_IF_NULL(ops, rxrpc);

    return ZX_OK;
}

zx_status_t devhost_device_install(zx_device_t* dev) {
    zx_status_t status;
    if ((status = device_validate(dev)) < 0) {
        dev->flags |= DEV_FLAG_DEAD | DEV_FLAG_VERY_DEAD;
        return status;
    }
    // Don't create an event handle if we already have one
    if ((dev->event == ZX_HANDLE_INVALID) &&
        ((status = zx_eventpair_create(0, &dev->event, &dev->local_event)) < 0)) {
        printf("device add: %p(%s): cannot create event: %d\n",
               dev, dev->name, status);
        dev->flags |= DEV_FLAG_DEAD | DEV_FLAG_VERY_DEAD;
        return status;
    }
    dev_ref_acquire(dev);
    dev->flags |= DEV_FLAG_ADDED;
    return ZX_OK;
}

zx_status_t devhost_device_add(zx_device_t* dev, zx_device_t* parent,
                               const zx_device_prop_t* props, uint32_t prop_count,
                               const char* proxy_args) TA_REQ(&__devhost_api_lock) {
    zx_status_t status;
    if ((status = device_validate(dev)) < 0) {
        goto fail;
    }
    if (parent == NULL) {
        printf("device_add: cannot add %p(%s) to NULL parent\n", dev, dev->name);
        status = ZX_ERR_NOT_SUPPORTED;
        goto fail;
    }
    if (parent->flags & DEV_FLAG_DEAD) {
        printf("device add: %p: is dead, cannot add child %p\n", parent, dev);
        status = ZX_ERR_BAD_STATE;
        goto fail;
    }

    creation_context_t* ctx = NULL;

    // if creation ctx (thread local) is set, we are in a thread
    // that is handling a bind() or create() callback and if that
    // ctx's parent matches the one provided to add we need to do
    // some additional checking...
    if ((creation_ctx != NULL) && (creation_ctx->parent == parent)) {
        ctx = creation_ctx;
        if (ctx->rpc != ZX_HANDLE_INVALID) {
            // create() must create only one child
            if (ctx->child != NULL) {
                printf("devhost: driver attempted to create multiple proxy devices!\n");
                return ZX_ERR_BAD_STATE;
            }
        }
    }

#if TRACE_ADD_REMOVE
    printf("devhost: device add: %p(%s) parent=%p(%s)\n",
            dev, dev->name, parent, parent->name);
#endif

    // Don't create an event handle if we alredy have one
    if ((dev->event == ZX_HANDLE_INVALID) &&
        ((status = zx_eventpair_create(0, &dev->event, &dev->local_event)) < 0)) {
        printf("device add: %p(%s): cannot create event: %d\n",
               dev, dev->name, status);
        goto fail;
    }

    dev->flags |= DEV_FLAG_BUSY;

    // this is balanced by end of devhost_device_remove
    // or, for instanced devices, by the last close
    dev_ref_acquire(dev);

    // proxy devices are created through this handshake process
    if (ctx && (ctx->rpc != ZX_HANDLE_INVALID)) {
        dev->flags |= DEV_FLAG_ADDED;
        dev->flags &= (~DEV_FLAG_BUSY);
        dev->rpc = ctx->rpc;
        ctx->child = dev;
        return ZX_OK;
    }

    dev_ref_acquire(parent);
    dev->parent = parent;

    if (dev->flags & DEV_FLAG_INSTANCE) {
        list_add_tail(&parent->instances, &dev->node);
    } else {
        // add to the device tree
        list_add_tail(&parent->children, &dev->node);

        // devhost_add always consumes the handle
        status = devhost_add(parent, dev, proxy_args, props, prop_count);
        if (status < 0) {
            printf("devhost: %p(%s): remote add failed %d\n",
                   dev, dev->name, status);
            dev_ref_release(dev->parent);
            dev->parent = NULL;
            dev_ref_release(dev);
            list_delete(&dev->node);
            dev->flags &= (~DEV_FLAG_BUSY);
            return status;
        }
    }
    dev->flags |= DEV_FLAG_ADDED;
    dev->flags &= (~DEV_FLAG_BUSY);

    // record this device in the creation context if there is one
    if (ctx && (ctx->child == NULL)) {
        ctx->child = dev;
    }
    return ZX_OK;

fail:
    dev->flags |= DEV_FLAG_DEAD | DEV_FLAG_VERY_DEAD;
    return status;
}

#define REMOVAL_BAD_FLAGS \
    (DEV_FLAG_DEAD | DEV_FLAG_BUSY |\
     DEV_FLAG_INSTANCE | DEV_FLAG_MULTI_BIND)

static const char* removal_problem(uint32_t flags) {
    if (flags & DEV_FLAG_DEAD) {
        return "already dead";
    }
    if (flags & DEV_FLAG_BUSY) {
        return "being created";
    }
    if (flags & DEV_FLAG_INSTANCE) {
        return "ephemeral device";
    }
    if (flags & DEV_FLAG_MULTI_BIND) {
        return "multi-bind-able device";
    }
    return "?";
}

static void devhost_unbind_child(zx_device_t* child) TA_REQ(&__devhost_api_lock) {
    // call child's unbind op
    if (child->ops->unbind) {
#if TRACE_ADD_REMOVE
        printf("call unbind child: %p(%s)\n", child, child->name);
#endif
        // hold a reference so the child won't get released during its unbind callback.
        dev_ref_acquire(child);
        DM_UNLOCK();
        dev_op_unbind(child);
        DM_LOCK();
        dev_ref_release(child);
    }
}

static void devhost_unbind_children(zx_device_t* dev) TA_REQ(&__devhost_api_lock) {
    zx_device_t* child = NULL;
    zx_device_t* temp = NULL;
#if TRACE_ADD_REMOVE
    printf("devhost_unbind_children: %p(%s)\n", dev, dev->name);
#endif
    list_for_every_entry_safe(&dev->children, child, temp, zx_device_t, node) {
        devhost_unbind_child(child);
    }
    list_for_every_entry_safe(&dev->instances, child, temp, zx_device_t, node) {
        devhost_unbind_child(child);
    }
}

zx_status_t devhost_device_remove(zx_device_t* dev) TA_REQ(&__devhost_api_lock) {
    if (dev->flags & REMOVAL_BAD_FLAGS) {
        printf("device: %p(%s): cannot be removed (%s)\n",
               dev, dev->name, removal_problem(dev->flags));
        return ZX_ERR_INVALID_ARGS;
    }
#if TRACE_ADD_REMOVE
    printf("device: %p(%s): is being removed\n", dev, dev->name);
#endif
    dev->flags |= DEV_FLAG_DEAD;

    devhost_unbind_children(dev);

    // cause the vfs entry to be unpublished to avoid further open() attempts
    xprintf("device: %p: devhost->devmgr remove rpc\n", dev);
    devhost_remove(dev);

    // detach from parent.  we do not downref the parent
    // until after our refcount hits zero and our release()
    // hook has been called.
    if (dev->parent) {
        list_delete(&dev->node);
    }

    dev->flags |= DEV_FLAG_VERY_DEAD;

    // this must be last, since it may result in the device structure being destroyed
    dev_ref_release(dev);

    return ZX_OK;
}

zx_status_t devhost_device_rebind(zx_device_t* dev) TA_REQ(&__devhost_api_lock) {
    dev->flags |= DEV_FLAG_BUSY;

    // remove children
    zx_device_t* child;
    zx_device_t* temp;
    list_for_every_entry_safe(&dev->children, child, temp, zx_device_t, node) {
        devhost_device_remove(child);
    }

    // notify children that they've been unbound
    devhost_unbind_children(dev);

    dev->flags &= ~DEV_FLAG_BUSY;

    // ask devcoord to find us a driver if it can
    devhost_device_bind(dev, "");
    return ZX_OK;
}

zx_status_t devhost_device_open_at(zx_device_t* dev, zx_device_t** out, const char* path, uint32_t flags)
    TA_REQ(&__devhost_api_lock) {
    if (dev->flags & DEV_FLAG_DEAD) {
        printf("device open: %p(%s) is dead!\n", dev, dev->name);
        return ZX_ERR_BAD_STATE;
    }
    dev_ref_acquire(dev);
    zx_status_t r;
    DM_UNLOCK();
    *out = dev;
    if (path) {
        r = dev_op_open_at(dev, out, path, flags);
    } else {
        r = dev_op_open(dev, out, flags);
    }
    DM_LOCK();
    if (r < 0) {
        dev_ref_release(dev);
    } else if (*out != dev) {
        // open created a per-instance device for us
        dev_ref_release(dev);

        dev = *out;
        if (!(dev->flags & DEV_FLAG_INSTANCE)) {
            printf("device open: %p(%s) in bad state %x\n", dev, dev->name, flags);
            panic();
        }
    }
    return r;
}

zx_status_t devhost_device_close(zx_device_t* dev, uint32_t flags) TA_REQ(&__devhost_api_lock) {
    zx_status_t r;
    DM_UNLOCK();
    r = dev_op_close(dev, flags);
    DM_LOCK();
    dev_ref_release(dev);
    return r;
}

zx_status_t devhost_device_suspend(zx_device_t* dev, uint32_t flags) {
    zx_status_t st;
    zx_device_t* child = NULL;
    list_for_every_entry(&dev->children, child, zx_device_t, node) {
        st = devhost_device_suspend(child, flags);
        if (st != ZX_OK) {
            return st;
        }
    }
    st = dev->ops->suspend(dev->ctx, flags);
    // default_suspend() returns ZX_ERR_NOT_SUPPORTED
    if ((st != ZX_OK) && (st != ZX_ERR_NOT_SUPPORTED)) {
        return st;
    } else {
        return ZX_OK;
    }
    return ZX_OK;
}

void devhost_device_destroy(zx_device_t* dev) {
    // Only destroy devices immediately after device_create() or after they're dead.
    ZX_DEBUG_ASSERT(dev->flags == 0 || dev->flags & DEV_FLAG_VERY_DEAD);
    dev->magic = 0xdeaddead;
    dev->ops = NULL;
    free(dev);
}

typedef struct {
    int fd;
    const char* path;
    int open_failures;
} fw_dir;

static fw_dir fw_dirs[] = {
    { -1, BOOT_FIRMWARE_DIR, 0},
    { -1, SYSTEM_FIRMWARE_DIR, 0},
};

static int devhost_open_firmware(const char* fwpath) {
    for (size_t i = 0; i < countof(fw_dirs); i++) {
        // Open the firmware directory if necessary
        if (fw_dirs[i].fd < 0) {
            fw_dirs[i].fd = open(fw_dirs[i].path, O_RDONLY | O_DIRECTORY);
            // If the directory doesn't open, it could mean there is no firmware
            // at that path (so the build system didn't create the directory),
            // or the filesystem hasn't been mounted yet. Log a warning every 5
            // failures and move on.
            if (fw_dirs[i].fd < 0) {
                if (fw_dirs[i].open_failures++ % 5 == 0) {
                    printf("devhost: warning: could not open firmware dir '%s' (err=%d)\n",
                            fw_dirs[i].path, errno);
                }
            }
        }
        // If the firmware directory is open, try to load the firmware.
        if (fw_dirs[i].fd >= 0) {
            int fwfd = openat(fw_dirs[i].fd, fwpath, O_RDONLY);
            // If the error is NOT that the firmware wasn't found, (e.g.,
            // EACCES), return early, with errno set by openat.
            if (fwfd >= 0 || errno != ENOENT) return fwfd;
        }
    }

    // Firmware wasn't found anywhere.
    errno = ENOENT;
    return -1;
}

zx_status_t devhost_load_firmware(zx_device_t* dev, const char* path, zx_handle_t* fw,
                                  size_t* size) {
    xprintf("devhost: dev=%p path=%s fw=%p\n", dev, path, fw);

    int fwfd = devhost_open_firmware(path);
    if (fwfd < 0) {
        switch (errno) {
        case ENOENT: return ZX_ERR_NOT_FOUND;
        case EACCES: return ZX_ERR_ACCESS_DENIED;
        case ENOMEM: return ZX_ERR_NO_MEMORY;
        default: return ZX_ERR_INTERNAL;
        }
    }

    struct stat fwstat;
    int ret = fstat(fwfd, &fwstat);
    if (ret < 0) {
        int e = errno;
        close(fwfd);
        switch (e) {
        case EACCES: return ZX_ERR_ACCESS_DENIED;
        case EBADF:
        case EFAULT: return ZX_ERR_BAD_STATE;
        default: return ZX_ERR_INTERNAL;
        }
    }

    if (fwstat.st_size == 0) {
        close(fwfd);
        return ZX_ERR_NOT_SUPPORTED;
    }

    uint64_t vmo_size = (fwstat.st_size + 4095) & ~4095;
    zx_handle_t vmo;
    zx_status_t status = zx_vmo_create(vmo_size, 0, &vmo);
    if (status != ZX_OK) {
        close(fwfd);
        return status;
    }

    uint8_t buffer[4096];
    size_t remaining = fwstat.st_size;
    uint64_t off = 0;
    while (remaining) {
        ssize_t r = read(fwfd, buffer, sizeof(buffer));
        if (r < 0) {
            close(fwfd);
            zx_handle_close(vmo);
            // Distinguish other errors?
            return ZX_ERR_IO;
        }
        if (r == 0) break;
        size_t actual = 0;
        status = zx_vmo_write(vmo, (const void*)buffer, off, (size_t)r, &actual);
        if (actual < (size_t)r) {
            printf("devhost: BUG: wrote %zu < %zu firmware vmo bytes!\n", actual, r);
            close(fwfd);
            zx_handle_close(vmo);
            return ZX_ERR_BAD_STATE;
        }
        off += actual;
        remaining -= actual;
    }

    if (remaining > 0) {
        printf("devhost: EOF found before writing firmware '%s'\n", path);
    }
    *fw = vmo;
    *size = fwstat.st_size;
    close(fwfd);

    return ZX_OK;
}

// Copyright 2018 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <dev/interrupt/arm_gic_hw_interface.h>
#include <vm/pmm.h>
#include <arch/arm64/hypervisor/gic/gicv2.h>
#include <dev/interrupt/arm_gicv2_regs.h>

// Representation of GICH registers.
typedef struct Gich {
    uint32_t hcr;
    uint32_t vtr;
    uint32_t vmcr;
    uint32_t reserved0;
    uint32_t misr;
    uint32_t reserved1[3];
    uint64_t eisr;
    uint32_t reserved2[2];
    uint64_t elrs;
    uint32_t reserved3[46];
    uint32_t apr;
    uint32_t reserved4[3];
    uint32_t lr[64];
} __attribute__((__packed__)) Gich;

static_assert(__offsetof(Gich, hcr) == 0x00, "");
static_assert(__offsetof(Gich, vtr) == 0x04, "");
static_assert(__offsetof(Gich, vmcr) == 0x08, "");
static_assert(__offsetof(Gich, misr) == 0x10, "");
static_assert(__offsetof(Gich, eisr) == 0x20, "");
static_assert(__offsetof(Gich, elrs) == 0x30, "");
static_assert(__offsetof(Gich, apr) == 0xf0, "");
static_assert(__offsetof(Gich, lr) == 0x100, "");

static volatile Gich* gich = NULL;

/* Returns the GICH_HCR value */
static uint32_t gicv2_read_gich_hcr(void) {
    return gich->hcr;
}

/* Writes to the GICH_HCR register */
static void gicv2_write_gich_hcr(uint32_t val) {
    gich->hcr = val;
}

/* Returns the GICH_VTR value */
static uint32_t gicv2_read_gich_vtr(void) {
    return gich->vtr;
}

/* Writes to the GICH_VTR register */
static void gicv2_write_gich_vtr(uint32_t val) {
    gich->vtr = val;
}

/* Returns the GICH_VMCR value */
static uint32_t gicv2_read_gich_vmcr(void) {
    return gich->vmcr;
}

/* Writes to the GICH_VMCR register */
static void gicv2_write_gich_vmcr(uint32_t val) {
    gich->vmcr = val;
}

/* Returns the GICH_ELRS value */
static uint64_t gicv2_read_gich_elrs(void) {
    return gich->elrs;
}

/* Writes to the GICH_ELRS register */
static void gicv2_write_gich_elrs(uint64_t val) {
    gich->elrs = val;
}

/* Returns the GICH_LRn value */
static uint32_t gicv2_read_gich_lr(uint32_t idx) {
    return gich->lr[idx];
}

/* Writes to the GICH_LR register */
static void gicv2_write_gich_lr(uint32_t idx, uint32_t val) {
    gich->lr[idx] = val;
}

static zx_status_t gicv2_get_gicv(paddr_t* gicv_paddr) {
    // Check for presence of GICv2 virtualisation extensions.
    if (GICV_OFFSET == 0)
        return ZX_ERR_NOT_SUPPORTED;
    *gicv_paddr = vaddr_to_paddr(reinterpret_cast<void*>(GICV_ADDRESS));
    return ZX_OK;
}

static const struct arm_gic_hw_interface_ops gic_hw_register_ops = {
    .read_gich_hcr = gicv2_read_gich_hcr,
    .write_gich_hcr = gicv2_write_gich_hcr,
    .read_gich_vtr = gicv2_read_gich_vtr,
    .write_gich_vtr = gicv2_write_gich_vtr,
    .read_gich_vmcr = gicv2_read_gich_vmcr,
    .write_gich_vmcr = gicv2_write_gich_vmcr,
    .read_gich_elrs = gicv2_read_gich_elrs,
    .write_gich_elrs = gicv2_write_gich_elrs,
    .read_gich_lr = gicv2_read_gich_lr,
    .write_gich_lr = gicv2_write_gich_lr,
    .get_gicv = gicv2_get_gicv,
};

void gicv2_hw_interface_register(void) {
    // Populate GICH
    gich = reinterpret_cast<volatile Gich*>(GICH_ADDRESS + 0x1000);
    arm_gic_hw_interface_register(&gic_hw_register_ops);
}

// Copyright 2016 The Fuchsia Authors
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT

#include <object/event_pair_dispatcher.h>

#include <assert.h>
#include <err.h>

#include <zircon/rights.h>
#include <fbl/alloc_checker.h>
#include <fbl/auto_lock.h>

constexpr uint32_t kUserSignalMask = ZX_EVENT_SIGNALED | ZX_USER_SIGNAL_ALL;

zx_status_t EventPairDispatcher::Create(fbl::RefPtr<Dispatcher>* dispatcher0,
                                        fbl::RefPtr<Dispatcher>* dispatcher1,
                                        zx_rights_t* rights) {
    fbl::AllocChecker ac;
    auto disp0 = fbl::AdoptRef(new (&ac) EventPairDispatcher());
    if (!ac.check())
        return ZX_ERR_NO_MEMORY;

    auto disp1 = fbl::AdoptRef(new (&ac) EventPairDispatcher());
    if (!ac.check())
        return ZX_ERR_NO_MEMORY;

    disp0->Init(disp1);
    disp1->Init(disp0);

    *rights = ZX_DEFAULT_EVENT_PAIR_RIGHTS;
    *dispatcher0 = fbl::move(disp0);
    *dispatcher1 = fbl::move(disp1);

    return ZX_OK;
}

EventPairDispatcher::~EventPairDispatcher() {}

void EventPairDispatcher::on_zero_handles() {
    canary_.Assert();

    fbl::AutoLock locker(&lock_);
    DEBUG_ASSERT(other_);

    other_->InvalidateCookie(other_->get_cookie_jar());
    other_->UpdateState(0u, ZX_EPAIR_PEER_CLOSED);
    other_.reset();
}

zx_status_t EventPairDispatcher::user_signal(uint32_t clear_mask, uint32_t set_mask, bool peer) {
    canary_.Assert();

    if ((set_mask & ~kUserSignalMask) || (clear_mask & ~kUserSignalMask))
        return ZX_ERR_INVALID_ARGS;

    if (!peer) {
        UpdateState(clear_mask, set_mask);
        return ZX_OK;
    }

    fbl::AutoLock locker(&lock_);
    // object_signal() may race with handle_close() on another thread.
    if (!other_)
        return ZX_ERR_PEER_CLOSED;
    other_->UpdateState(clear_mask, set_mask);
    return ZX_OK;
}

EventPairDispatcher::EventPairDispatcher()
        : other_koid_(0ull) {}

// This is called before either EventPairDispatcher is accessible from threads other than the one
// initializing the event pair, so it does not need locking.
void EventPairDispatcher::Init(fbl::RefPtr<EventPairDispatcher> other) TA_NO_THREAD_SAFETY_ANALYSIS {
    DEBUG_ASSERT(other);
    // No need to take |lock_| here.
    DEBUG_ASSERT(!other_);
    other_koid_ = other->get_koid();
    other_ = fbl::move(other);
}

// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ddktl/device-internal.h>
#include <fbl/type_support.h>
#include <fbl/unique_ptr.h>
#include <zircon/types.h>

#include <stdint.h>

namespace ddk {

class WlanmacIfcProxy;

namespace internal {

DECLARE_HAS_MEMBER_FN(has_wlanmac_status, WlanmacStatus);
DECLARE_HAS_MEMBER_FN(has_wlanmac_recv, WlanmacRecv);
DECLARE_HAS_MEMBER_FN(has_wlanmac_complete_tx, WlanmacCompleteTx);

template <typename D>
constexpr void CheckWlanmacIfc() {
    static_assert(internal::has_wlanmac_status<D>::value,
                  "WlanmacIfc subclasses must implement WlanmacStatus");
    static_assert(fbl::is_same<decltype(&D::WlanmacStatus),
                               void (D::*)(uint32_t)>::value,
                  "WlanmacStatus must be a non-static member function with signature "
                  "'void WlanmacStatus(uint32_t)', and be visible to ddk::WlanmacIfc<D> "
                  "(either because they are public, or because of friendship).");
    static_assert(internal::has_wlanmac_recv<D>::value,
                  "WlanmacIfc subclasses must implement WlanmacRecv");
    static_assert(fbl::is_same<decltype(&D::WlanmacRecv),
                               void (D::*)(uint32_t, const void*, size_t, wlan_rx_info_t*)>::value,
                  "WlanmacRecv must be a non-static member function with signature "
                  "'void WlanmacRecv(uint32_t, const void*, size_t, wlan_rx_info_t*)', and be "
                  "visible to ddk::WlanmacIfc<D> (either because they are public, or because of "
                  "friendship).");
    static_assert(internal::has_wlanmac_complete_tx<D>::value,
                  "WlanmacIfc subclasses must implement WlanmacCompleteTx");
    static_assert(fbl::is_same<decltype(&D::WlanmacCompleteTx),
                               void (D::*)(wlan_tx_packet_t*, zx_status_t)>::value,
                  "WlanmacCompleteTx must be a non-static member function with signature "
                  "'void WlanmacCompleteTx(wlan_tx_packet_t*, zx_status_t)', and be "
                  "visible to ddk::WlanmacIfc<D> (either because they are public, or because of "
                  "friendship).");
}

DECLARE_HAS_MEMBER_FN(has_wlanmac_query, WlanmacQuery);
DECLARE_HAS_MEMBER_FN(has_wlanmac_stop, WlanmacStop);
DECLARE_HAS_MEMBER_FN(has_wlanmac_start, WlanmacStart);
DECLARE_HAS_MEMBER_FN(has_wlanmac_queue_tx, WlanmacQueueTx);
DECLARE_HAS_MEMBER_FN(has_wlanmac_set_channel, WlanmacSetChannel);
DECLARE_HAS_MEMBER_FN(has_wlanmac_set_bss, WlanmacSetBss);
DECLARE_HAS_MEMBER_FN(has_wlanmac_configure_bss, WlanmacConfigureBss);
DECLARE_HAS_MEMBER_FN(has_wlanmac_set_key, WlanmacSetKey);

template <typename D>
constexpr void CheckWlanmacProtocolSubclass() {
    static_assert(internal::has_wlanmac_query<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacQuery");
    static_assert(fbl::is_same<decltype(&D::WlanmacQuery),
                               zx_status_t (D::*)(uint32_t, wlanmac_info_t*)>::value,
                  "WlanmacQuery must be a non-static member function with signature "
                  "'zx_status_t WlanmacQuery(uint32_t, wlanmac_info_t*)', and be visible to "
                  "ddk::WlanmacProtocol<D> (either because they are public, or because of "
                  "friendship).");
    static_assert(internal::has_wlanmac_stop<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacStop");
    static_assert(fbl::is_same<decltype(&D::WlanmacStop),
                               void (D::*)()>::value,
                  "WlanmacStop must be a non-static member function with signature "
                  "'void WlanmacStop()', and be visible to ddk::WlanmacProtocol<D> (either "
                  "because they are public, or because of friendship).");
    static_assert(internal::has_wlanmac_start<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacStart");
    static_assert(fbl::is_same<decltype(&D::WlanmacStart),
                               zx_status_t (D::*)(fbl::unique_ptr<WlanmacIfcProxy>)>::value,
                  "WlanmacStart must be a non-static member function with signature "
                  "'zx_status_t WlanmacStart(fbl::unique_ptr<WlanmacIfcProxy>)', and be visible "
                  "to ddk::WlanmacProtocol<D> (either because they are public, or because of "
                  "friendship).");
    static_assert(internal::has_wlanmac_queue_tx<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacQueueTx");
    static_assert(fbl::is_same<decltype(&D::WlanmacQueueTx),
                               zx_status_t (D::*)(uint32_t, wlan_tx_packet_t*)>::value,
                  "WlanmacQueueTx must be a non-static member function with signature "
                  "'zx_status_t WlanmacQueueTx(uint32_t, wlan_tx_packet_t*)', and be "
                  "visible to ddk::WlanmacProtocol<D> (either because they are public, or because "
                  "of friendship).");
    static_assert(internal::has_wlanmac_set_channel<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacSetChannel");
    static_assert(fbl::is_same<decltype(&D::WlanmacSetChannel),
                               zx_status_t (D::*)(uint32_t, wlan_channel_t*)>::value,
                  "WlanmacSetChannel must be a non-static member function with signature "
                  "'zx_status_t WlanmacSetChannel(uint32_t, wlan_channel_t*)', and be visible to "
                  "ddk::WlanmacProtocol<D> (either because they are public, or because of "
                  "friendship).");
    static_assert(internal::has_wlanmac_set_bss<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacSetBss");
    static_assert(fbl::is_same<decltype(&D::WlanmacSetBss),
                               zx_status_t (D::*)(uint32_t, const uint8_t[6], uint8_t)>::value,
                  "WlanmacSetBss must be a non-static member function with signature "
                  "'zx_status_t WlanmacSetBss(uint32_t, const uint8_t[6], uint8_t)', and be visible to "
                  "ddk::WlanmacProtocol<D> (either because they are public, or because of "
                  "friendship).");
    static_assert(internal::has_wlanmac_configure_bss<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacConfigureBss");
    static_assert(fbl::is_same<decltype(&D::WlanmacConfigureBss),
                               zx_status_t (D::*)(uint32_t, wlan_bss_config_t*)>::value,
                  "WlanmacSetBss must be a non-static member function with signature "
                  "'zx_status_t WlanmacconfigureBss(uint32_t, wlan_bss_config_t*)', and be visible to "
                  "ddk::WlanmacProtocol<D> (either because they are public, or because of "
                  "friendship).");
    static_assert(internal::has_wlanmac_set_key<D>::value,
                  "WlanmacProtocol subclasses must implement WlanmacSetKey");
    static_assert(fbl::is_same<decltype(&D::WlanmacSetKey),
                               zx_status_t (D::*)(uint32_t, wlan_key_config_t*)>::value,
                  "WlanmacSetKey must be a non-static member function with signature "
                  "'zx_status_t WlanmacSetKey(uint32_t, wlan_key_config_t*)', and be visible to "
                  "ddk::WlanmacProtocol<D> (either because they are public, or because of "
                  "friendship).");
}

} // namespace internal
} // namespace ddk

// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <ddktl/device.h>
#include <ddktl/protocol/wlan.h>
#include <fbl/alloc_checker.h>
#include <fbl/unique_ptr.h>
#include <unittest/unittest.h>

namespace {

// These tests are testing interfaces that get included via multiple inheritance, and thus we must
// make sure we get all the casts correct. We record the value of the "this" pointer in the
// constructor, and then verify in each call the "this" pointer was the same as the original. (The
// typical way for this to go wrong is to take a WlanmacIfc<D>* instead of a D* in a function
// signature.)
#define get_this() reinterpret_cast<uintptr_t>(this)

class TestWlanmacIfc : public ddk::Device<TestWlanmacIfc>,
                       public ddk::WlanmacIfc<TestWlanmacIfc> {
public:
    TestWlanmacIfc()
        : ddk::Device<TestWlanmacIfc>(nullptr) {
        this_ = get_this();
    }

    void DdkRelease() {}

    void WlanmacStatus(uint32_t status) {
        status_this_ = get_this();
        status_called_ = true;
    }

    void WlanmacRecv(uint32_t flags, const void* data, size_t length, wlan_rx_info_t* info) {
        recv_this_ = get_this();
        recv_called_ = true;
    }

    void WlanmacCompleteTx(wlan_tx_packet_t* pkt, zx_status_t status) {
        complete_tx_this_ = get_this();
        complete_tx_called_ = true;
    }

    bool VerifyCalls() const {
        BEGIN_HELPER;
        EXPECT_EQ(this_, status_this_, "");
        EXPECT_EQ(this_, recv_this_, "");
        EXPECT_EQ(this_, complete_tx_this_, "");
        EXPECT_TRUE(status_called_, "");
        EXPECT_TRUE(recv_called_, "");
        EXPECT_TRUE(complete_tx_called_, "");
        END_HELPER;
    }

    zx_status_t StartProtocol(ddk::WlanmacProtocolProxy* proxy) {
        return proxy->Start(this);
    }

private:
    uintptr_t this_ = 0u;
    uintptr_t status_this_ = 0u;
    uintptr_t recv_this_ = 0u;
    uintptr_t complete_tx_this_ = 0u;
    bool status_called_ = false;
    bool recv_called_ = false;
    bool complete_tx_called_ = false;
};

class TestWlanmacProtocol : public ddk::Device<TestWlanmacProtocol, ddk::GetProtocolable>,
                            public ddk::WlanmacProtocol<TestWlanmacProtocol> {
public:
    TestWlanmacProtocol()
        : ddk::Device<TestWlanmacProtocol, ddk::GetProtocolable>(nullptr) {
        this_ = get_this();
    }

    zx_status_t DdkGetProtocol(uint32_t proto_id, void* out) {
        if (proto_id != ZX_PROTOCOL_WLANMAC)
            return ZX_ERR_INVALID_ARGS;
        ddk::AnyProtocol* proto = static_cast<ddk::AnyProtocol*>(out);
        proto->ops = ddk_proto_ops_;
        proto->ctx = this;
        return ZX_OK;
    }

    void DdkRelease() {}

    zx_status_t WlanmacQuery(uint32_t options, wlanmac_info_t* info) {
        query_this_ = get_this();
        query_called_ = true;
        return ZX_OK;
    }

    void WlanmacStop() {
        stop_this_ = get_this();
        stop_called_ = true;
    }

    zx_status_t WlanmacStart(fbl::unique_ptr<ddk::WlanmacIfcProxy> proxy) {
        start_this_ = get_this();
        proxy_.swap(proxy);
        start_called_ = true;
        return ZX_OK;
    }

    zx_status_t WlanmacQueueTx(uint32_t options, wlan_tx_packet_t* pkt) {
        queue_tx_this_ = get_this();
        queue_tx_called_ = true;
        return ZX_OK;
    }

    zx_status_t WlanmacSetChannel(uint32_t options, wlan_channel_t* chan) {
        set_channel_this_ = get_this();
        set_channel_called_ = true;
        return ZX_OK;
    }

    zx_status_t WlanmacSetBss(uint32_t options, const uint8_t mac[6], uint8_t type) {
        set_bss_this_ = get_this();
        set_bss_called_ = true;
        return ZX_OK;
    }

    zx_status_t WlanmacConfigureBss(uint32_t options, wlan_bss_config_t* config) {
        configure_bss_this_ = get_this();
        configure_bss_called_ = true;
        return ZX_OK;
    }

    zx_status_t WlanmacSetKey(uint32_t options, wlan_key_config_t* key_config) {
        set_key_this_ = get_this();
        set_key_called_ = true;
        return ZX_OK;
    }

    bool VerifyCalls() const {
        BEGIN_HELPER;
        EXPECT_EQ(this_, query_this_, "");
        EXPECT_EQ(this_, start_this_, "");
        EXPECT_EQ(this_, stop_this_, "");
        EXPECT_EQ(this_, queue_tx_this_, "");
        EXPECT_EQ(this_, set_channel_this_, "");
        EXPECT_EQ(this_, set_bss_this_, "");
        EXPECT_EQ(this_, configure_bss_this_, "");
        EXPECT_EQ(this_, set_key_this_, "");
        EXPECT_TRUE(query_called_, "");
        EXPECT_TRUE(start_called_, "");
        EXPECT_TRUE(stop_called_, "");
        EXPECT_TRUE(queue_tx_called_, "");
        EXPECT_TRUE(set_channel_called_, "");
        EXPECT_TRUE(set_bss_called_, "");
        EXPECT_TRUE(configure_bss_called_, "");
        EXPECT_TRUE(set_key_called_, "");
        END_HELPER;
    }

    bool TestIfc() {
        if (!proxy_)
            return false;
        // Use the provided proxy to test the ifc proxy.
        proxy_->Status(0);
        proxy_->Recv(0, nullptr, 0, nullptr);
        proxy_->CompleteTx(nullptr, ZX_OK);
        return true;
    }

private:
    uintptr_t this_ = 0u;
    uintptr_t query_this_ = 0u;
    uintptr_t stop_this_ = 0u;
    uintptr_t start_this_ = 0u;
    uintptr_t queue_tx_this_ = 0u;
    uintptr_t set_channel_this_ = 0u;
    uintptr_t set_bss_this_ = 0u;
    uintptr_t configure_bss_this_ = 0u;
    uintptr_t set_key_this_ = 0u;
    bool query_called_ = false;
    bool stop_called_ = false;
    bool start_called_ = false;
    bool queue_tx_called_ = false;
    bool set_channel_called_ = false;
    bool set_bss_called_ = false;
    bool configure_bss_called_ = false;
    bool set_key_called_ = false;

    fbl::unique_ptr<ddk::WlanmacIfcProxy> proxy_;
};

static bool test_wlanmac_ifc() {
    BEGIN_TEST;

    TestWlanmacIfc dev;

    auto ifc = dev.wlanmac_ifc();
    ifc->status(&dev, 0);
    ifc->recv(&dev, 0, nullptr, 0, nullptr);
    ifc->complete_tx(&dev, nullptr, ZX_OK);

    EXPECT_TRUE(dev.VerifyCalls(), "");

    END_TEST;
}

static bool test_wlanmac_ifc_proxy() {
    BEGIN_TEST;

    TestWlanmacIfc dev;
    ddk::WlanmacIfcProxy proxy(dev.wlanmac_ifc(), &dev);

    proxy.Status(0);
    proxy.Recv(0, nullptr, 0, nullptr);
    proxy.CompleteTx(nullptr, ZX_OK);

    EXPECT_TRUE(dev.VerifyCalls(), "");

    END_TEST;
}

static bool test_wlanmac_protocol() {
    BEGIN_TEST;

    TestWlanmacProtocol dev;

    // Normally we would use device_op_get_protocol, but we haven't added the device to devmgr so
    // its ops table is currently invalid.
    wlanmac_protocol_t proto;
    auto status = dev.DdkGetProtocol(0, reinterpret_cast<void*>(&proto));
    EXPECT_EQ(ZX_ERR_INVALID_ARGS, status, "");

    status = dev.DdkGetProtocol(ZX_PROTOCOL_WLANMAC, reinterpret_cast<void*>(&proto));
    EXPECT_EQ(ZX_OK, status, "");
    EXPECT_EQ(ZX_OK, proto.ops->query(proto.ctx, 0, nullptr), "");
    proto.ops->stop(proto.ctx);
    EXPECT_EQ(ZX_OK, proto.ops->start(proto.ctx, nullptr, nullptr), "");
    proto.ops->queue_tx(proto.ctx, 0, nullptr);
    EXPECT_EQ(ZX_OK, proto.ops->set_channel(proto.ctx, 0, nullptr), "");
    EXPECT_EQ(ZX_OK, proto.ops->set_bss(proto.ctx, 0, nullptr, 0), "");
    EXPECT_EQ(ZX_OK, proto.ops->configure_bss(proto.ctx, 0, nullptr), "");
    EXPECT_EQ(ZX_OK, proto.ops->set_key(proto.ctx, 0, nullptr), "");

    EXPECT_TRUE(dev.VerifyCalls(), "");

    END_TEST;
}

static bool test_wlanmac_protocol_proxy() {
    BEGIN_TEST;

    // The WlanmacProtocol device to wrap. This would live in the parent device
    // our driver was binding to.
    TestWlanmacProtocol protocol_dev;

    wlanmac_protocol_t proto;
    auto status = protocol_dev.DdkGetProtocol(ZX_PROTOCOL_WLANMAC, reinterpret_cast<void*>(&proto));
    EXPECT_EQ(ZX_OK, status, "");
    // The proxy device to wrap the ops + device that represent the parent
    // device.
    ddk::WlanmacProtocolProxy proxy(&proto);
    // The WlanmacIfc to hand to the parent device.
    TestWlanmacIfc ifc_dev;

    EXPECT_EQ(ZX_OK, proxy.Query(0, nullptr), "");
    proxy.Stop();
    EXPECT_EQ(ZX_OK, proxy.Start(&ifc_dev), "");
    proxy.QueueTx(0, nullptr);
    proxy.SetChannel(0, nullptr);
    proxy.SetBss(0, nullptr, 0);
    proxy.ConfigureBss(0, nullptr);
    proxy.SetKey(0, nullptr);

    EXPECT_TRUE(protocol_dev.VerifyCalls(), "");

    END_TEST;
}

static bool test_wlanmac_protocol_ifc_proxy() {
    BEGIN_TEST;

    // We create a protocol device that we will start from an ifc device. The protocol device will
    // then use the pointer passed to it to call methods on the ifc device. This ensures the void*
    // casting is correct.
    TestWlanmacProtocol protocol_dev;

    wlanmac_protocol_t proto;
    auto status = protocol_dev.DdkGetProtocol(ZX_PROTOCOL_WLANMAC, reinterpret_cast<void*>(&proto));
    EXPECT_EQ(ZX_OK, status, "");

    ddk::WlanmacProtocolProxy proxy(&proto);
    TestWlanmacIfc ifc_dev;
    EXPECT_EQ(ZX_OK, ifc_dev.StartProtocol(&proxy), "");

    // Execute the WlanmacIfc methods
    ASSERT_TRUE(protocol_dev.TestIfc(), "");
    // Verify that they were called
    EXPECT_TRUE(ifc_dev.VerifyCalls(), "");

    END_TEST;
}

} // namespace

BEGIN_TEST_CASE(ddktl_wlan_device)
RUN_NAMED_TEST("ddk::WlanmacIfc", test_wlanmac_ifc);
RUN_NAMED_TEST("ddk::WlanmacIfcProxy", test_wlanmac_ifc_proxy);
RUN_NAMED_TEST("ddk::WlanmacProtocol", test_wlanmac_protocol);
RUN_NAMED_TEST("ddk::WlanmacProtocolProxy", test_wlanmac_protocol_proxy);
RUN_NAMED_TEST("WlanmacProtocol using WlanmacIfcProxy", test_wlanmac_protocol_ifc_proxy);
END_TEST_CASE(ddktl_wlan_device)

test_case_element* test_case_ddktl_wlan_device = TEST_CASE_ELEMENT(ddktl_wlan_device);

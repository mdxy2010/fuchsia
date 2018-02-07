// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <zircon/compiler.h>

#include <ddk/device.h>
#include <ddk/protocol/block.h>
#include <ddk/protocol/sdmmc.h>
#include <hw/sdmmc.h>

#include <threads.h>

__BEGIN_CDECLS;

typedef enum sdmmc_type {
    SDMMC_TYPE_SD,
    SDMMC_TYPE_MMC,
} sdmmc_type_t;

#define SDMMC_REQ_COUNT   16

typedef struct sdmmc_device {
    zx_device_t* zxdev;

    sdmmc_protocol_t host;
    sdmmc_host_info_t host_info;

    sdmmc_type_t type;

    sdmmc_bus_width_t bus_width;
    sdmmc_voltage_t signal_voltage;
    sdmmc_timing_t timing;

    unsigned clock_rate;    // Bus clock rate
    uint64_t capacity;      // Card capacity

    uint16_t rca;           // Relative address

    uint32_t raw_cid[4];
    uint32_t raw_csd[4];
    uint8_t raw_ext_csd[512];

    mtx_t lock;

    // blockio requests
    list_node_t txn_list;

    // outstanding request (1 right now)
    sdmmc_req_t req;

    thrd_t worker_thread;
    zx_handle_t worker_event;
    bool worker_thread_running;

    block_info_t block_info;
} sdmmc_device_t;

// SD/MMC shared ops

zx_status_t sdmmc_go_idle(sdmmc_device_t* dev);
zx_status_t sdmmc_send_status(sdmmc_device_t* dev, uint32_t* response);
zx_status_t sdmmc_stop_transmission(sdmmc_device_t* dev);

// SD ops

zx_status_t sd_send_if_cond(sdmmc_device_t* dev);

// MMC ops

zx_status_t mmc_send_op_cond(sdmmc_device_t* dev, uint32_t ocr, uint32_t* rocr);
zx_status_t mmc_all_send_cid(sdmmc_device_t* dev, uint32_t cid[4]);
zx_status_t mmc_set_relative_addr(sdmmc_device_t* dev, uint16_t rca);
zx_status_t mmc_send_csd(sdmmc_device_t* dev, uint32_t csd[4]);
zx_status_t mmc_send_ext_csd(sdmmc_device_t* dev, uint8_t ext_csd[512]);
zx_status_t mmc_select_card(sdmmc_device_t* dev);
zx_status_t mmc_switch(sdmmc_device_t* dev, uint8_t index, uint8_t value);

zx_status_t sdmmc_probe_sd(sdmmc_device_t* dev);
zx_status_t sdmmc_probe_mmc(sdmmc_device_t* dev);

__END_CDECLS;

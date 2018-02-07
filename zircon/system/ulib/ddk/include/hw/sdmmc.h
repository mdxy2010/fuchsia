// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#pragma once

#include <ddk/device.h>
#include <zircon/compiler.h>

__BEGIN_CDECLS;

//
// Common SD/MMC defines
//
#define SDHC_BLOCK_SIZE 512

#define SDMMC_RESP_MASK      (0x3 << 16)
#define SDMMC_RESP_LEN_EMPTY (0x0 << 16)
#define SDMMC_RESP_LEN_136   (0x1 << 16)
#define SDMMC_RESP_LEN_48    (0x2 << 16)
#define SDMMC_RESP_LEN_48B   (0x3 << 16)

#define SDMMC_RESP_CRC_CHECK     (0x1 << 19)
#define SDMMC_RESP_CMD_IDX_CHECK (0x1 << 20)
#define SDMMC_RESP_DATA_PRESENT  (0x1 << 21)
#define SDMMC_CMD_TYPE_NORMAL    (0x0 << 22)
#define SDMMC_CMD_TYPE_SUSPEND   (0x1 << 22)
#define SDMMC_CMD_TYPE_RESUME    (0x2 << 22)
#define SDMMC_CMD_TYPE_ABORT     (0x3 << 22)

#define SDMMC_RESP_NONE (0x0)
#define SDMMC_RESP_R1   (SDMMC_RESP_LEN_48 | SDMMC_RESP_CMD_IDX_CHECK | SDMMC_RESP_CRC_CHECK)
#define SDMMC_RESP_R1b  (SDMMC_RESP_LEN_48B | SDMMC_RESP_CMD_IDX_CHECK | SDMMC_RESP_CRC_CHECK)
#define SDMMC_RESP_R2   (SDMMC_RESP_LEN_136 | SDMMC_RESP_CRC_CHECK)
#define SDMMC_RESP_R3   (SDMMC_RESP_LEN_48)
#define SDMMC_RESP_R4   (SDMMC_RESP_LEN_48)
#define SDMMC_RESP_R5   (SDMMC_RESP_LEN_48 | SDMMC_RESP_CMD_IDX_CHECK | SDMMC_RESP_CRC_CHECK)
#define SDMMC_RESP_R5b  (SDMMC_RESP_LEN_48B | SDMMC_RESP_CMD_IDX_CHECK | SDMMC_RESP_CRC_CHECK)
#define SDMMC_RESP_R6   (SDMMC_RESP_LEN_48 | SDMMC_RESP_CMD_IDX_CHECK | SDMMC_RESP_CRC_CHECK)
#define SDMMC_RESP_R7   (SDMMC_RESP_LEN_48 | SDMMC_RESP_CMD_IDX_CHECK | SDMMC_RESP_CRC_CHECK)

#define SDMMC_CMD_DMA_EN    (1 << 0)
#define SDMMC_CMD_BLKCNT_EN (1 << 1)
#define SDMMC_CMD_AUTO12    (1 << 2)
#define SDMMC_CMD_AUTO23    (2 << 2)
#define SDMMC_CMD_READ      (1 << 4)
#define SDMMC_CMD_MULTI_BLK (1 << 5)

#define SDMMC_COMMAND(c) ((c) << 24)

// Common SD/MMC commands
#define SDMMC_GO_IDLE_STATE           (SDMMC_COMMAND(0))
#define SDMMC_ALL_SEND_CID            (SDMMC_COMMAND(2) | SDMMC_RESP_R2)
#define SDMMC_SEND_CSD                (SDMMC_COMMAND(9) | SDMMC_RESP_R2)
#define SDMMC_STOP_TRANSMISSION       (SDMMC_COMMAND(12) | SDMMC_RESP_R1b | SDMMC_CMD_TYPE_ABORT)
#define SDMMC_SEND_STATUS             (SDMMC_COMMAND(13) | SDMMC_RESP_R1)
#define SDMMC_READ_BLOCK              (SDMMC_COMMAND(17) | SDMMC_RESP_R1 | \
                                       SDMMC_RESP_DATA_PRESENT | SDMMC_CMD_READ)
#define SDMMC_READ_MULTIPLE_BLOCK     (SDMMC_COMMAND(18) | SDMMC_RESP_R1 | \
                                       SDMMC_RESP_DATA_PRESENT | SDMMC_CMD_READ | \
                                       SDMMC_CMD_MULTI_BLK | SDMMC_CMD_BLKCNT_EN)
#define SDMMC_WRITE_BLOCK             (SDMMC_COMMAND(24) | SDMMC_RESP_R1 | \
                                       SDMMC_RESP_DATA_PRESENT)
#define SDMMC_WRITE_MULTIPLE_BLOCK    (SDMMC_COMMAND(25) | SDMMC_RESP_R1 | \
                                       SDMMC_RESP_DATA_PRESENT | SDMMC_CMD_MULTI_BLK | \
                                       SDMMC_CMD_BLKCNT_EN)
#define SDMMC_LOCK_UNLOCK             (SDMMC_COMMAND(42) | SDMMC_RESP_R1)
#define SDMMC_APP_CMD                 (SDMMC_COMMAND(55) | SDMMC_RESP_R1)
#define SDMMC_GEN_CMD                 (SDMMC_COMMAND(56) | SDMMC_RESP_R1 | SD_CMD_ISDATA)

// SD Commands
#define SD_SEND_RELATIVE_ADDR         (SDMMC_COMMAND(3) | SDMMC_RESP_R6)
#define SD_SWITCH_FUNC                (SDMMC_COMMAND(6) | SDMMC_RESP_R1)
#define SD_SELECT_CARD                (SDMMC_COMMAND(7) | SDMMC_RESP_R1b)
#define SD_SEND_IF_COND               (SDMMC_COMMAND(8) | SDMMC_RESP_R7)
#define SD_VOLTAGE_SWITCH             (SDMMC_COMMAND(11) | SDMMC_RESP_R1)
#define SD_APP_SEND_SCR               (SDMMC_COMMAND(51) | SDMMC_RESP_R1 | \
                                       SDMMC_RESP_DATA_PRESENT | SDMMC_CMD_READ)
// MMC Commands
#define MMC_SEND_OP_COND              (SDMMC_COMMAND(1) | SDMMC_RESP_R3)
#define MMC_SET_RELATIVE_ADDR         (SDMMC_COMMAND(3) | SDMMC_RESP_R1)
#define MMC_SWITCH                    (SDMMC_COMMAND(6) | SDMMC_RESP_R1b)
#define MMC_SELECT_CARD               (SDMMC_COMMAND(7) | SDMMC_RESP_R1)
#define MMC_SEND_EXT_CSD              (SDMMC_COMMAND(8) | SDMMC_RESP_R1 | \
                                       SDMMC_RESP_DATA_PRESENT | SDMMC_CMD_READ)
#define MMC_SEND_TUNING_BLOCK         (SDMMC_COMMAND(21) | SDMMC_RESP_R1 | \
                                       SDMMC_RESP_DATA_PRESENT | SDMMC_CMD_READ)

// CID fields (SD/MMC)
#define MMC_CID_SPEC_VRSN_40    3

// OCR fields (MMC)
#define MMC_OCR_BUSY            (1 << 31)

// EXT_CSD fields (MMC)
#define MMC_EXT_CSD_BUS_WIDTH   183
#define MMC_EXT_CSD_BUS_WIDTH_8_DDR 6
#define MMC_EXT_CSD_BUS_WIDTH_4_DDR 5
#define MMC_EXT_CSD_BUS_WIDTH_8     2
#define MMC_EXT_CSD_BUS_WIDTH_4     1
#define MMC_EXT_CSD_BUS_WIDTH_1     0

#define MMC_EXT_CSD_HS_TIMING   185
#define MMC_EXT_CSD_HS_TIMING_LEGACY    0
#define MMC_EXT_CSD_HS_TIMING_HS        1
#define MMC_EXT_CSD_HS_TIMING_HS200     2
#define MMC_EXT_CSD_HS_TIMING_HS400     3

#define MMC_EXT_CSD_DEVICE_TYPE 196

// Device register (CMD13 response) fields (SD/MMC)
#define MMC_STATUS_ADDR_OUT_OF_RANGE    (1 << 31)
#define MMC_STATUS_ADDR_MISALIGN        (1 << 30)
#define MMC_STATUS_BLOCK_LEN_ERR        (1 << 29)
#define MMC_STATUS_ERASE_SEQ_ERR        (1 << 28)
#define MMC_STATUS_ERASE_PARAM          (1 << 27)
#define MMC_STATUS_WP_VIOLATION         (1 << 26)
#define MMC_STATUS_DEVICE_LOCKED        (1 << 25)
#define MMC_STATUS_LOCK_UNLOCK_FAILED   (1 << 24)
#define MMC_STATUS_COM_CRC_ERR          (1 << 23)
#define MMC_STATUS_ILLEGAL_COMMAND      (1 << 22)
#define MMC_STATUS_DEVICE_ECC_FAILED    (1 << 21)
#define MMC_STATUS_CC_ERR               (1 << 20)
#define MMC_STATUS_ERR                  (1 << 19)
#define MMC_STATUS_CXD_OVERWRITE        (1 << 16)
#define MMC_STATUS_WP_ERASE_SKIP        (1 << 15)
#define MMC_STATUS_ERASE_RESET          (1 << 13)
#define MMC_STATUS_CURRENT_STATE_MASK   (0xf << 9)
#define MMC_STATUS_CURRENT_STATE(resp)  ((resp) & MMC_STATUS_CURRENT_STATE_MASK)
#define MMC_STATUS_CURRENT_STATE_TRAN   (0x4 << 9)
#define MMC_STATUS_CURRENT_STATE_RECV   (0x5 << 9)
#define MMC_STATUS_CURRENT_STATE_DATA   (0x6 << 9)
#define MMC_STATUS_READY_FOR_DATA       (1 << 8)
#define MMC_STATUS_SWITCH_ERR           (1 << 7)
#define MMC_STATUS_EXCEPTION_EVENT      (1 << 6)
#define MMC_STATUS_APP_CMD              (1 << 5)

__END_CDECLS;

# Copyright 2016 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_TYPE := usertest

MODULE_USERTEST_GROUP := core

MODULE_SRCS += \
    $(LOCAL_DIR)/channel.c \

MODULE_NAME := channel-test

MODULE_LIBS := \
    system/ulib/unittest system/ulib/fdio system/ulib/zircon system/ulib/c

# We need a header file generated by kernel/lib/vdso/rules.mk.
MODULE_COMPILEFLAGS += -I$(BUILDDIR)/kernel/lib/vdso
MODULE_SRCDEPS += $(BUILDDIR)/kernel/lib/vdso/vdso-code.h

include make/module.mk

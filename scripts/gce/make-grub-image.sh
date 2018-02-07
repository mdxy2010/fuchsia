#!/bin/bash
# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [[ -z $FUCHSIA_GCE_PROJECT ]]; then
  source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"/env.sh
fi

grubdisk="$FUCHSIA_OUT_DIR/${FUCHSIA_GCE_GRUB}.raw"

makefile 1m "$grubdisk"

$FUCHSIA_DIR/scripts/grubdisk/build-all.sh "$grubdisk"

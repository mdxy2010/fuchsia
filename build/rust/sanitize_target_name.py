#!/usr/bin/env python
# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys


def main():
    print(sys.argv[1].replace("-", "_"))
    return 0


if __name__ == '__main__':
    sys.exit(main())

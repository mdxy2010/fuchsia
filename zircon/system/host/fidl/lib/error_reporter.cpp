// Copyright 2017 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "error_reporter.h"

namespace fidl {

void ErrorReporter::ReportError(StringView error) {
    errors_.push_back(error);
}

void ErrorReporter::PrintReports() {
    for (const auto& error : errors_) {
        fprintf(stderr, "%s", error.data());
    }
}

} // namespace fidl

# Copyright 2017 The Fuchsia Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

if [[ -n "${ZSH_VERSION}" ]]; then
  devshell_lib_dir=${${(%):-%x}:a:h}
else
  devshell_lib_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
fi

export FUCHSIA_DIR="$(dirname $(dirname $(dirname "${devshell_lib_dir}")))"
export FUCHSIA_OUT_DIR="${FUCHSIA_OUT_DIR:-${FUCHSIA_DIR}/out}"
export FUCHSIA_CONFIG="${FUCHSIA_CONFIG:-${FUCHSIA_DIR}/.config}"
unset devshell_lib_dir

export ZIRCON_TOOLS_DIR="${FUCHSIA_OUT_DIR}/build-zircon/tools"

if [[ "${FUCHSIA_DEVSHELL_VERBOSITY}" -eq 1 ]]; then
  set -x
fi

function fx-config-read-if-present {
  if [[ ! -f "${FUCHSIA_CONFIG}" ]]; then
    return 1
  fi

  source "${FUCHSIA_CONFIG}"

  # Paths are relative to FUCHSIA_DIR unless they're absolute paths.
  if [[ "${FUCHSIA_BUILD_DIR:0:1}" == "/" ]]; then
    export FUCHSIA_BUILD_DIR="${FUCHSIA_BUILD_DIR}"
  else
    export FUCHSIA_BUILD_DIR="${FUCHSIA_DIR}/${FUCHSIA_BUILD_DIR}"
  fi

  export FUCHSIA_VARIANT="${FUCHSIA_VARIANT}"
  export FUCHSIA_ARCH="${FUCHSIA_ARCH}"
  export ZIRCON_PROJECT="${ZIRCON_PROJECT}"

  export ZIRCON_BUILDROOT="${ZIRCON_BUILDROOT:-${FUCHSIA_OUT_DIR}/build-zircon}"
  export ZIRCON_BUILD_DIR="${ZIRCON_BUILD_DIR:-${ZIRCON_BUILDROOT}/build-${ZIRCON_PROJECT}}"
  return 0
}

function fx-config-read {
  if ! fx-config-read-if-present ; then
    echo >& 2 "error: Cannot read config from ${FUCHSIA_CONFIG}. Did you run \"fx set\"?"
    exit 1
  fi
}

function fx-command-run {
  local -r command_name="$1"
  local -r command_path="${FUCHSIA_DIR}/scripts/devshell/${command_name}"

  if [[ ! -f "${command_path}" ]]; then
    echo >& 2 "error: Unknown command ${command_name}"
    exit 1
  fi

  shift
  "${command_path}" "$@"
}

function fx-print-command-help {
  local -r command_path="$1"
  if grep '^## ' "$command_path" > /dev/null; then
    sed -n -e 's/^## //p' -e 's/^##$//p' < "$command_path"
  else
    local -r command_name=$(basename "$command_path")
    echo "No help found. Try \`fx $command_name -h\`"
  fi
}

function fx-command-help {
  fx-print-command-help "$0"
}

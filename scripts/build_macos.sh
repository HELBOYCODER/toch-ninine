#!/usr/bin/env bash
# Configures and builds the portable core. Fails on any compiler warning.
set -euo pipefail

cd "$(dirname "$0")/.."
LOG="${BUILD_LOG:-build_macos.log}"

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release 2>&1 | tee "$LOG"

if grep -q "warning:" "$LOG"; then
    echo "error: the build produced compiler warnings (see $LOG)" >&2
    exit 1
fi

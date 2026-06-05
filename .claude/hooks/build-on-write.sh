#!/usr/bin/env bash
set -uo pipefail

BUILD_PRESET="clang-ninja-debug"
PROJECT_ROOT="$(git rev-parse --show-toplevel)"

cd "$PROJECT_ROOT"

run_build() {
    cmake --build --preset "$BUILD_PRESET" 2>&1
}

output=$(run_build) || {
    # On Windows, clangd may hold a .pcm file memory-mapped when the build tries
    # to overwrite it. Wait and retry once before giving up.
    if echo "$output" | grep -q "user-mapped section"; then
        sleep 3
        output=$(run_build) || true
    fi
}

echo "$output" | grep -E "(error|warning):" | head -40 || true

#!/usr/bin/env bash
set -euo pipefail

BUILD_PRESET="clang-ninja-debug"
PROJECT_ROOT="$(git rev-parse --show-toplevel)"

cd "$PROJECT_ROOT"

cmake --build --preset "$BUILD_PRESET" 2>&1 | \
  grep -E "(error|warning):" | \
  head -40 || true
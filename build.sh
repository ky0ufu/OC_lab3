#!/usr/bin/env bash
set -e

BUILD_DIR=build

mkdir -p ${BUILD_DIR}
cd ${BUILD_DIR}

cmake ..

cmake --build . -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

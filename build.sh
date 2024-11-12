#!/usr/bin/env bash

set -eux

if [ "$#" -lt 3 ]; then
    echo "Usage: $0 WINDOW_SYSTEM (x11, wayland) VIDEO_ENC (gpu-intel, cpu) BUILD_DIR"
    exit 1
fi

WINDOW_SYSTEM="$1"
VIDEO_ENC="$2"
BUILD_DIR="$3"

BUILD_NUM_WORKERS=8
BUILD_TYPE=Release

apt-get install -y --no-install-recommends \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev

cd "$WINDOW_SYSTEM" \
    && cmake -E make_directory "build" \
    && cmake \
        -DCMAKE_INSTALL_PREFIX="$BUILD_DIR" \
        -DVIDEO_ENC="$VIDEO_ENC" \
        -S . -B "build" \
    && cmake --build "build" --config "$BUILD_TYPE" --parallel $BUILD_NUM_WORKERS

mkdir -p "$BUILD_DIR/bin" && cp build/src/streamd "$BUILD_DIR/bin/streamd"

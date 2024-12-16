#!/usr/bin/env bash

set -eux

WINDOW_SYSTEM="x11"
VIDEO_ENC="cpu"
BUILD_DIR="/tmp/build"
LOG_LEVEL=2

while [ $# -gt 0 ]; do
  case "$1" in
    --window-system*|-u*)
      if [[ "$1" != *=* ]]; then shift; fi
      WINDOW_SYSTEM="${1#*=}"
      ;;
    --video-enc*|-f*)
      if [[ "$1" != *=* ]]; then shift; fi
      VIDEO_ENC="${1#*=}"
      ;;
    --build-dir*|-f*)
      if [[ "$1" != *=* ]]; then shift; fi
      BUILD_DIR="${1#*=}"
      ;;
    --log-level*|-f*)
      if [[ "$1" != *=* ]]; then shift; fi
      LOG_LEVEL=${1#*=}
      ;;
    --help|-h)
      printf "Usage: $0 --window-system [x11,wayland] --video-enc [gpu-intel,cpu] --build-dir [path] --log-level [0,1,2,3] (0 for error, 3 for debug)"
      exit 0
      ;;
    *)
      >&2 printf "Error: Invalid argument\n"
      exit 1
      ;;
  esac
  shift
done

BUILD_NUM_WORKERS=8
BUILD_TYPE=Release

cd "$WINDOW_SYSTEM" \
    && cmake -E make_directory "build" \
    && cmake \
        -DCMAKE_INSTALL_PREFIX="$BUILD_DIR" \
        -DVIDEO_ENC="$VIDEO_ENC" \
        -DLOG_LEVEL=$LOG_LEVEL \
        -S . -B "build" \
    && cmake --build "build" --config "$BUILD_TYPE" --parallel $BUILD_NUM_WORKERS

mkdir -p "$BUILD_DIR/bin" && cp build/src/streamd "$BUILD_DIR/bin/streamd"

#!/bin/bash

set -e  # stop on error

BUILD_DIR="build"
EXE_NAME="rubberduck"
EXE_PATH="$(pwd)/$BUILD_DIR/$EXE_NAME"

# === Create build directory if missing ===
if [ ! -d "$BUILD_DIR" ]; then
    echo "[INFO] Creating build directory..."
    mkdir "$BUILD_DIR"
fi

# === Configure if not already configured ===
if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    echo "[INFO] Configuring project..."
    cmake -S . -B "$BUILD_DIR"
fi

# === Build (incremental) ===
cmake --build "$BUILD_DIR"

# === If no script provided, stop here ===
if [ -z "$1" ]; then
    echo "[INFO] Build completed. No script provided."
    exit 0
fi

# === Check RD script existence ===
if [ ! -f "$1" ]; then
    echo "[ERROR] Script file \"$1\" not found!"
    exit 1
fi

# === Run the script ===
"$EXE_PATH" "$1"
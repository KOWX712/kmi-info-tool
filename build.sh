#!/bin/bash

# set -e

DIR=$(pwd)
OUTDIR="$DIR/out"

if [ -d "/opt/android-ndk" ]; then
    echo "Using system NDK path: /opt/android-ndk"
    NDK_HOME="/opt/android-ndk"
else
    echo "Using default NDK path: $DIR/android-ndk"
    NDK_HOME="$DIR/android-ndk"
fi

if [ ! -d "$NDK_HOME" ]; then
    LATEST_NDK_URL=$(curl -s https://developer.android.com/ndk/downloads | grep -oP 'https://dl.google.com/android/repository/android-ndk-r[0-9]+[a-z]?-linux\.zip' | head -n 1)
    [ -z "$LATEST_NDK_URL" ] && LATEST_NDK_URL="https://dl.google.com/android/repository/android-ndk-r27c-linux.zip"
    NDK_ZIP="android-ndk.zip"
    wget "$LATEST_NDK_URL" -O "$NDK_ZIP"
    TEMP_DIR=$(mktemp -d)
    unzip "$NDK_ZIP" -d "$TEMP_DIR"
    EXTRACTED_DIR=$(find "$TEMP_DIR" -maxdepth 1 -name "android-ndk-*" -type d | head -n 1)
    mv -f "$EXTRACTED_DIR" "$NDK_HOME"

    # Clean up
    rm -rf "$TEMP_DIR" "$NDK_ZIP"
fi

mkdir -p "$OUTDIR"
rm -rf "$OUTDIR/"*

# Build for android arm64
$NDK_HOME/toolchains/llvm/prebuilt/linux-x86_64/bin/aarch64-linux-android21-clang \
    -Wall -O2 -fPIC -fPIE -pie \
    -DANDROID -DARM64 \
    -o "$OUTDIR/kmi" main.c

[ -f "$OUTDIR/kmi" ] && echo "Build complete: kmi"

# Build for Linux x86_64
gcc -Wall -O2 -fPIC -fPIE -pie \
    -DLINUX -DX86_64 \
    -o "$OUTDIR/kmi-linux" main.c

[ -f "$OUTDIR/kmi-linux" ] && echo "Build complete: kmi-linux"

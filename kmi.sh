#!/bin/sh

# Simpliflied shell version, no magiskboot built-in

PATH=/data/adb/ap/bin:/data/adb/ksu/bin:/data/adb/magisk:$PATH
for i in /data/adb/magisk /data/adb/ksu/bin /data/adb/ap/bin; do
    [ -f "$i/magiskboot" ] && chmod +x $i/magiskboot && break
done

if ! command -v magiskboot >/dev/null 2>&1; then
    echo "Error: magiskboot not found!"
    exit 1
fi

if [ -z "$1" ]; then
    if ! command -v getprop >/dev/null 2>&1; then
        echo "Error: boot image not found, please provide /path/to/image."
        exit 1
    fi
    IMG="/dev/block/by-name/boot$(getprop ro.boot.slot_suffix)"
else
    IMG="$1"
fi

# Unpack boot image
magiskboot unpack "$IMG" > info 2>&1

# Decompress kernel
magiskboot decompress kernel >/dev/null 2>&1

# Get security patch level and kernel compression format
SECURITY_PATH=$(grep "OS_PATCH_LEVEL" info | awk -F'[][]' '{print $2}')
COMPRESSION=$(grep "KERNEL_FMT" info | awk -F'[][]' '{print $2}')

if [ "$COMPRESSION" = "raw" ]; then
    COMPRESSION=""
else
    COMPRESSION="-$COMPRESSION"
fi

# Get kernel version
KERNEL_VER_RAW=$(strings kernel | grep "Linux version" | sort -r | head -n1)
if echo "$KERNEL_VER_RAW" | grep -q "android"; then
    KERNEL_VER=$(echo "$KERNEL_VER_RAW" | awk -F'[- ]' '{print $4 "-" $3}')
else
    KERNEL_VER=$(echo "$KERNEL_VER_RAW" | cut -d'[- ]' -f3)
fi

echo "KMI: ${KERNEL_VER}_${SECURITY_PATH}-boot${COMPRESSION}"

# Clean up
magiskboot cleanup >/dev/null 2>&1
rm -f info

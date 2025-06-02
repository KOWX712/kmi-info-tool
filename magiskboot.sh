#!/bin/bash

# This script is use to update magiskboot binary

MAGIISK_LATEST=$(curl -Ls https://api.github.com/repos/topjohnwu/Magisk/releases/latest | jq -r '.assets[].browser_download_url')

wget -q "$MAGIISK_LATEST" -O magisk.apk

# Extract and compress arm64-v8a binary
unzip -p magisk.apk 'lib/arm64-v8a/libmagiskboot.so' > libmagiskboot_arm64.so
chmod +x libmagiskboot_arm64.so && upx -q --best --lzma libmagiskboot_arm64.so
xxd -i -n magiskboot libmagiskboot_arm64.so > magiskboot_arm64-v8a.h

# Extract and compress x86_64 binary
unzip -p magisk.apk 'lib/x86_64/libmagiskboot.so' > libmagiskboot_x86_64.so
chmod +x libmagiskboot_x86_64.so && upx -q --best --lzma libmagiskboot_x86_64.so
xxd -i -n magiskboot libmagiskboot_x86_64.so > magiskboot_x86_64.h

rm -f magisk.apk libmagiskboot_arm64.so libmagiskboot_x86_64.so

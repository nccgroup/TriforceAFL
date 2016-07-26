#!/bin/sh
#
# build triforce patched qemu for AFL
#

cd qemu

CFLAGS="-O3" ./configure --disable-werror \
  --enable-system --enable-linux-user \
  --enable-guest-base --disable-gtk --disable-sdl --disable-vnc \
  --target-list="x86_64-linux-user x86_64-softmmu arm-softmmu"

make
cp -f "x86_64-linux-user/qemu-x86_64" "../../afl-qemu-trace"
cp -f "x86_64-softmmu/qemu-system-x86_64" "../../afl-qemu-system-trace"
cp -f "arm-softmmu/qemu-system-arm" "../../qemu-system-arm"

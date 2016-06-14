#!/bin/sh
#
# build triforce patched qemu for AFL
#

CPU_TARGET=x86_64
cd qemu

CFLAGS="-O3" ./configure --disable-werror \
  --enable-system --enable-linux-user \
  --enable-guest-base --disable-gtk --disable-sdl --disable-vnc \
  --target-list="${CPU_TARGET}-linux-user ${CPU_TARGET}-softmmu"

make
cp -f "${CPU_TARGET}-linux-user/qemu-${CPU_TARGET}" "../../afl-qemu-trace"
cp -f "${CPU_TARGET}-softmmu/qemu-system-${CPU_TARGET}" "../../afl-qemu-system-trace"

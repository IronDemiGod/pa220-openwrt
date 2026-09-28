#!/bin/sh
#
# Set up an OpenWrt build tree for the Palo Alto Networks PA-220.
#
#   ./setup.sh [directory]      (default: ./openwrt-pa220)
#
# Clones OpenWrt v25.12.4, copies the PA-220 files from this folder over
# it, installs the feeds and writes the build configuration. Then:
#   cd <directory> && make -j$(nproc)
# Images end up in bin/targets/octeon/generic/:
#   *-initramfs-kernel.bin     RAM image (TFTP boot from U-Boot)
#   *-targz-sysupgrade.tar     install / upgrade image
#
set -e

OPENWRT_URL=https://git.openwrt.org/openwrt/openwrt.git
OPENWRT_TAG=v25.12.4

here=$(cd "$(dirname "$0")" && pwd)
dir=${1:-openwrt-pa220}

if [ ! -d "$dir/.git" ]; then
	git clone --branch "$OPENWRT_TAG" --depth 1 "$OPENWRT_URL" "$dir"
fi
cd "$dir"

# PA-220 files (same paths as in the OpenWrt tree)
(cd "$here" && tar cf - --exclude=./setup.sh .) | tar xf -

./scripts/feeds update -a
./scripts/feeds install -a

cp pa220.diffconfig .config
make defconfig

echo
echo "Ready. Build with:  cd $dir && make -j$(nproc)"

#!/bin/sh
#
# Build U-Boot for the PA-220.
#   ./build.sh /path/to/tools-gcc-4.7
# The argument is the OCTEON SDK toolchain directory (the one containing
# bin/mips64-octeon-linux-gnu-gcc). Without it the compiler must already
# be in PATH.
#
set -e
cd "$(dirname "$0")"

if [ -n "$1" ]; then
	PATH="$(cd "$1" && pwd)/bin:$PATH"
fi
command -v mips64-octeon-linux-gnu-gcc >/dev/null || {
	echo "mips64-octeon-linux-gnu-gcc not found (pass the tools-gcc-4.7 directory)" >&2
	exit 1
}
export PATH OCTEON_MODEL=OCTEON_CN70XX

# The 2013 host tools need an old C dialect with a current gcc.
HOSTCC="$(pwd)/tools/hostcc-gnu89"

make octeon_pa220_config
make -j"$(nproc)" HOSTCC="$HOSTCC"
ls -l u-boot-octeon_pa220.bin

#!/bin/sh
#
# Set up an OpenWrt build tree for the Palo Alto Networks PA-220.
#
#   ./setup.sh [directory]          first time (default: ./openwrt-pa220):
#                                   clone OpenWrt v25.12.6, copy the PA-220
#                                   files over it, apply patches/*.patch to
#                                   OpenWrt's own files, install the feeds,
#                                   write the build configuration
#   ./setup.sh --sync <directory>   after changing files here: copy them into
#                                   an existing tree again (only changed
#                                   files are touched, files removed here are
#                                   removed there / restored to OpenWrt's
#                                   version) and rewrite the configuration;
#                                   no clone, no feed update
#
# Then build:  cd <directory> && make -j$(nproc)
# Images end up in bin/targets/octeon/generic/:
#   *-initramfs-kernel.bin     RAM image (TFTP boot from U-Boot)
#   *-targz-sysupgrade.tar     install / upgrade image
#
set -e
export LC_ALL=C

OPENWRT_URL=https://git.openwrt.org/openwrt/openwrt.git
OPENWRT_MIRROR=https://github.com/openwrt/openwrt.git	# official mirror
OPENWRT_TAG=v25.12.6
MANIFEST=.pa220-files		# list of copied files, kept in the tree

here=$(cd "$(dirname "$0")" && pwd)

sync=0
if [ "$1" = "--sync" ]; then
	sync=1
	shift
	[ -n "$1" ] || { echo "usage: $0 --sync <directory>" >&2; exit 1; }
fi
dir=${1:-openwrt-pa220}

if [ $sync = 1 ]; then
	[ -f "$dir/scripts/feeds" ] || { echo "$dir is not an OpenWrt tree" >&2; exit 1; }
elif [ ! -d "$dir/.git" ]; then
	git clone --branch "$OPENWRT_TAG" --depth 1 "$OPENWRT_URL" "$dir" || {
		echo "$OPENWRT_URL failed, trying the mirror $OPENWRT_MIRROR"
		rm -rf "$dir"
		git clone --branch "$OPENWRT_TAG" --depth 1 "$OPENWRT_MIRROR" "$dir"
	}
fi
dir=$(cd "$dir" && pwd)

# The PA-220 file set is put together in $stage first:
#  - the files here (same paths as in the OpenWrt tree), except setup.sh,
#    patches/ and the build tree itself if it is inside this folder
#  - OpenWrt's own files changed by patches/*.patch: OpenWrt's version
#    with the patches applied, so OpenWrt's later changes to those files
#    are kept; a patch that no longer applies stops here
skip=./setup.sh
case "$dir" in
"$here"/*) skip=./${dir#"$here"/} ;;
esac
stage=$(mktemp -d)
trap 'rm -rf "$stage"' EXIT
(cd "$here" && find . \( -path "$skip" -o -path ./patches \) -prune -o \
	-type f ! -path ./setup.sh -print | tar cf - -T -) | (cd "$stage" && tar xf -)

for p in "$here"/patches/*.patch; do
	[ -f "$p" ] || continue
	sed -n 's|^+++ b/||p' "$p" | while read -r f; do
		[ -e "$stage/$f" ] && continue	# already there (earlier patch)
		mode=$(git -C "$dir" ls-tree "$OPENWRT_TAG" -- "$f" | cut -d' ' -f1)
		[ -n "$mode" ] || continue	# new file, created by the patch
		mkdir -p "$stage/$(dirname "$f")"
		git -C "$dir" show "$OPENWRT_TAG:$f" > "$stage/$f"
		[ "$mode" = 100755 ] && chmod 755 "$stage/$f"
		:
	done
	(cd "$stage" && git apply "$p") || {
		echo "${p##*/} does not apply to OpenWrt $OPENWRT_TAG, update it" >&2
		exit 1
	}
done
new_list=$(cd "$stage" && find . -type f | sed 's|^\./||' | sort)

# files copied last time that are gone here: restore OpenWrt's version
# or remove them
if [ -f "$dir/$MANIFEST" ]; then
	echo "$new_list" | comm -23 "$dir/$MANIFEST" - | while read -r f; do
		if git -C "$dir" cat-file -e "$OPENWRT_TAG:$f" 2>/dev/null; then
			echo "restoring OpenWrt's $f"
			git -C "$dir" show "$OPENWRT_TAG:$f" > "$dir/$f"
		elif [ -e "$dir/$f" ]; then
			echo "removing $f"
			rm -f "$dir/$f"
		fi
	done
fi

if command -v rsync >/dev/null; then
	# checksum compare: unchanged files keep their time stamps, so make
	# only rebuilds what really changed
	(cd "$stage" && echo "$new_list" | rsync -rlpc --files-from=- . "$dir/")
else
	(cd "$stage" && echo "$new_list" | tar cf - -T -) | (cd "$dir" && tar xf -)
fi
echo "$new_list" > "$dir/$MANIFEST"

cd "$dir"
if [ $sync = 0 ]; then
	./scripts/feeds update -a
	./scripts/feeds install -a
fi

cp pa220.diffconfig .config
make defconfig

echo
echo "Ready. Build with:  cd $dir && make -j$(nproc)"

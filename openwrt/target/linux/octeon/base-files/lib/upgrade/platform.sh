#
# Copyright (C) 2021 OpenWrt.org
#

if [ -x /usr/sbin/blkid ]; then
  RAMFS_COPY_BIN="/usr/sbin/blkid"
fi

platform_get_rootfs() {
	local rootfsdev
	local rootpartuuid

	if read cmdline < /proc/cmdline; then
		case "$cmdline" in
			*root=PARTUUID=*)
				rootpartuuid="${cmdline##*root=PARTUUID=}"
				rootpartuuid="${rootpartuuid%% *}"
				rootfsdev="$(blkid -o device -t PARTUUID="${rootpartuuid}")"
			;;
			*root=*)
				rootfsdev="${cmdline##*root=}"
				rootfsdev="${rootfsdev%% *}"
			;;
		esac

		echo "${rootfsdev}"
	fi
}

platform_get_n821_disk() {
	local partnum=$1
	local DEVNAME
	while read line; do
		export -n "${line}"
	done < $(find /sys/bus/platform/devices/16f0000000000.ehci/ -path \*block/sd[a-z]/uevent)
	echo "/dev/${DEVNAME}${partnum}"
}

platform_copy_config_helper() {
	local device=$1
	local fstype=$2

	mount -t "${fstype}" "$device" /mnt
	cp -af "$UPGRADE_BACKUP" "/mnt/$BACKUP_FILE"
	umount /mnt
}

platform_copy_config() {
	case "$(board_name)" in
	ubnt,erlite|\
	ubnt,usg)
		platform_copy_config_helper /dev/sda1 vfat
		;;
	itus,shield-router)
		platform_copy_config_helper /dev/mmcblk1p1 vfat
		;;
	er|\
	ubnt,edgerouter-4|\
	ubnt,edgerouter-6p)
		platform_copy_config_helper /dev/mmcblk0p1 vfat
		;;
	cisco,vedge1000)
		platform_copy_config_helper "$(platform_get_n821_disk 1)" ext2
		;;
	pan,pa-220)
		# restored by preinit (/sysupgrade.tgz on the new root)
		platform_copy_config_helper "$PA220_ROOTDEV" ext4
		;;
	esac
}

platform_do_flash() {
	local tar_file=$1
	local board=$2
	local kernel=$3
	local rootfs=$4

	local board_dir=$(tar tf "$tar_file" | grep -m 1 '^sysupgrade-.*/$')
	board_dir=${board_dir%/}
	[ -n "$board_dir" ] || return 1

	mkdir -p /boot

	if [ $board = "itus,shield-router" ]; then
		# mmcblk1p1 (fat) contains all ELF-bin images for the Shield
		mount /dev/mmcblk1p1 /boot

		echo "flashing Itus Kernel to /boot/$kernel (/dev/mmblk1p1)"
		tar -Oxf $tar_file "$board_dir/kernel" > /boot/$kernel
	else
		if [ "${board}" = "cisco,vedge1000" ]; then
			local rootpartuuid
			rootpartuuid="$(/usr/sbin/blkid -o value -s PARTUUID "${rootfs}")"
			if [ -n "${rootpartuuid}" ]; then
				echo "setting root partition to PARTUUID=${rootpartuuid}"
				fw_setenv bootcmd 'usb start; ext2load usb 0:1 $loadaddr vmlinux.64; bootoctlinux $loadaddr coremask=f endbootargs rootfstype=squashfs rootwait root=PARTUUID='"${rootpartuuid}"
			else
				echo "WARNING: unable to figure out root partition UUID, leaving bootcmd unchanged"
			fi
			mount -t ext2 "${kernel}" /boot
		else
			mount -t vfat "${kernel}" /boot
		fi

		[ -f /boot/vmlinux.64 -a ! -L /boot/vmlinux.64 ] && {
			mv /boot/vmlinux.64 /boot/vmlinux.64.previous
			mv /boot/vmlinux.64.md5 /boot/vmlinux.64.md5.previous
		}

		echo "flashing kernel to $(awk '/\/boot/ {print $1}' /proc/mounts)"
		tar xf $tar_file $board_dir/kernel -O > /boot/vmlinux.64
		md5sum /boot/vmlinux.64 | cut -f1 -d " " > /boot/vmlinux.64.md5
	fi

	echo "flashing rootfs to ${rootfs}"
	tar xf $tar_file $board_dir/root -O | dd of="${rootfs}" bs=4096

	sync
	umount /boot
}

# PA-220: eMMC p1 (ext3) holds the kernel that U-Boot loads
# (/vmlinux.oct3-mp), p2 is the ext4 root filesystem. The sysupgrade
# image is the "targz" one: kernel ELF + root filesystem as .tar.gz.
# The layout itself is created by /usr/sbin/pa220-install (RAM image).
PA220_BOOTDEV=/dev/mmcblk0p1
PA220_ROOTDEV=/dev/mmcblk0p2

pa220_do_upgrade() {
	local tar_file="$1"
	local board_dir=$(tar tf "$tar_file" | grep -m 1 '^sysupgrade-.*/$')
	local kdir=/boot
	local newroot=/tmp/pa220-newroot
	board_dir=${board_dir%/}
	[ -n "$board_dir" ] || return 1

	# Kernel: write it next to the old one first, then swap, keeping the
	# previous kernel as vmlinux.oct3-mp.bak (bootable from the U-Boot shell).
	mkdir -p /boot
	mount "$PA220_BOOTDEV" /boot || return 1
	mkdir -p "$kdir"
	echo "flashing kernel to $PA220_BOOTDEV:/vmlinux.oct3-mp"
	tar xf "$tar_file" "$board_dir/kernel" -O > "$kdir/vmlinux.oct3-mp.new"
	if [ ! -s "$kdir/vmlinux.oct3-mp.new" ]; then
		echo "kernel extraction failed, keeping the old kernel"
		rm -f "$kdir/vmlinux.oct3-mp.new"
		umount /boot
		return 1
	fi
	[ -f "$kdir/vmlinux.oct3-mp" ] && mv "$kdir/vmlinux.oct3-mp" "$kdir/vmlinux.oct3-mp.bak"
	mv "$kdir/vmlinux.oct3-mp.new" "$kdir/vmlinux.oct3-mp"
	sync
	umount /boot

	# Root: fresh ext4, then unpack the new root filesystem.
	echo "formatting $PA220_ROOTDEV and unpacking the root filesystem"
	mkfs.ext4 -F -q -L rootfs "$PA220_ROOTDEV" || return 1
	mkdir -p "$newroot"
	mount -t ext4 "$PA220_ROOTDEV" "$newroot" || return 1
	# busybox tar decompresses itself (no separate gzip needed in the ramfs)
	tar xf "$tar_file" "$board_dir/root" -O | tar xzf - -C "$newroot" || {
		echo "unpacking the root filesystem failed"
		umount "$newroot"
		return 1
	}
	mkdir -p "$newroot/boot"
	sync
	umount "$newroot"
	return 0
}

platform_do_upgrade() {
	local tar_file="$1"
	local board=$(board_name)
	local rootfs="$(platform_get_rootfs)"
	local kernel=

	if [ "$board" = "pan,pa-220" ]; then
		pa220_do_upgrade "$tar_file"
		return $?
	fi

	if [ ! -b "${rootfs}" ] && [ "${board}" = "cisco,vedge1000" ]; then
		# Default to the built-in USB disk for N821
		rootfs="$(platform_get_n821_disk 2)"
	fi
	[ -b "${rootfs}" ] || return 1
	case "$board" in
	er | \
	ubnt,edgerouter-4 | \
	ubnt,edgerouter-6p)
		kernel=/dev/mmcblk0p1
		;;
	ubnt,erlite|\
	ubnt,usg)
		kernel=/dev/sda1
		;;
	itus,shield-router)
		kernel=ItusrouterImage
		;;
	cisco,vedge1000)
		kernel="$(platform_get_n821_disk 1)"
		;;
	*)
		return 1
	esac

	platform_do_flash $tar_file $board $kernel $rootfs

	return 0
}

platform_check_image() {
	local board=$(board_name)
	local tar_file="$1"

	local board_dir=$(tar tf "$tar_file" | grep -m 1 '^sysupgrade-.*/$')
	board_dir=${board_dir%/}
	[ -n "$board_dir" ] || return 1

	case "$board" in
	er | \
	itus,shield-router | \
	ubnt,edgerouter-4 | \
	ubnt,edgerouter-6p | \
	ubnt,erlite | \
	ubnt,usg | \
	cisco,vedge1000)
		local kernel_length=$(tar xf $tar_file $board_dir/kernel -O | wc -c 2> /dev/null)
		local rootfs_length=$(tar xf $tar_file $board_dir/root -O | wc -c 2> /dev/null)
		[ "$kernel_length" = 0 -o "$rootfs_length" = 0 ] && {
			echo "The upgrade image is corrupt."
			return 1
		}
		return 0
		;;
	esac

	if [ "$board" = "pan,pa-220" ]; then
		# this image expects p1 = boot, p2 = root
		[ -b /dev/mmcblk0p2 ] && [ ! -b /dev/mmcblk0p3 ] || {
			echo "The eMMC does not have the PA-220 layout (p1 boot, p2 root). Install with pa220-install from the RAM image."
			return 1
		}
		local kmagic=$(tar xf "$tar_file" $board_dir/kernel -O 2>/dev/null | head -c 4 | hexdump -v -e '4/1 "%02x"')
		local rmagic=$(tar xf "$tar_file" $board_dir/root -O 2>/dev/null | head -c 2 | hexdump -v -e '2/1 "%02x"')
		[ "$kmagic" = "7f454c46" ] || {
			echo "The upgrade image has no valid kernel."
			return 1
		}
		[ "$rmagic" = "1f8b" ] || {
			echo "Use the *-targz-sysupgrade.tar image on the PA-220 (ext4 root), not the squashfs one."
			return 1
		}
		return 0
	fi

	echo "Sysupgrade is not yet supported on $board."
	return 1
}

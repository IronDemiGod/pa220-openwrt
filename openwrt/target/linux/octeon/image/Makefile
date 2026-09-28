# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2009-2010 OpenWrt.org

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/image.mk

define Build/append-dtb-to-elf
	$(TARGET_CROSS)objcopy --update-section .appended_dtb=$(KDIR)/image-$(DEVICE_DTS).dtb $@
endef

define Build/strip-kernel
	# Workaround pre-SDK-1.9.0 u-boot versions not handling the .notes section
	$(TARGET_CROSS)strip -R .notes $@ -o $@.stripped && mv $@.stripped $@
endef

DTS_DIR := $(DTS_DIR)/cavium-octeon

define Device/Default
  PROFILES = Default $$(DEVICE_NAME)
  KERNEL_NAME := vmlinux.elf
  KERNEL_INITRAMFS_NAME := vmlinux-initramfs.elf
  KERNEL := kernel-bin | strip-kernel | patch-cmdline
  IMAGES := sysupgrade.tar
  IMAGE/sysupgrade.tar/squashfs := append-rootfs | pad-extra 128k | sysupgrade-tar rootfs=$$$$@
  IMAGE/sysupgrade.tar := sysupgrade-tar
endef

define Device/generic
  DEVICE_VENDOR := Generic
  DEVICE_MODEL := Octeon
  FILESYSTEMS := ext4
endef
TARGET_DEVICES += generic

ITUSROUTER_CMDLINE:=console=ttyS0,115200 root=/dev/mmcblk1p2 rootfstype=squashfs,ext4,f2fs rootwait
define Device/itus_shield-router
  DEVICE_VENDOR := Itus Networks
  DEVICE_MODEL := Shield Router
  CMDLINE := $(ITUSROUTER_CMDLINE)
  IMAGE/sysupgrade.tar/squashfs += | append-metadata
endef
TARGET_DEVICES += itus_shield-router

# Disable PCIe on ER as it doesn't have PCIe peripherals and some devices lock up on initialization
ER_CMDLINE:=-mtdparts=phys_mapped_flash:640k(boot0)ro,640k(boot1)ro,64k(eeprom)ro root=/dev/mmcblk0p2 rootfstype=squashfs,ext4 rootwait pcie_octeon.pcie_disable=1
define Device/ubnt_edgerouter
  DEVICE_VENDOR := Ubiquiti
  DEVICE_MODEL := EdgeRouter
  BOARD_NAME := er
  CMDLINE := $(ER_CMDLINE)
  SUPPORTED_DEVICES += er
endef
TARGET_DEVICES += ubnt_edgerouter

define Device/ubnt_edgerouter-e300
  DEVICE_VENDOR := Ubiquiti
  DEVICE_PACKAGES += kmod-gpio-button-hotplug kmod-leds-gpio kmod-of-mdio kmod-sfp kmod-usb3 kmod-usb-dwc3 kmod-usb-storage-uas
  KERNEL := kernel-bin | patch-cmdline | append-dtb-to-elf
  KERNEL_DEPENDS := $$(wildcard $(DTS_DIR)/$(DEVICE_DTS).dts)
  CMDLINE := root=/dev/mmcblk0p2 rootfstype=squashfs,ext4 rootwait
endef

define Device/ubnt_edgerouter-4
  $(Device/ubnt_edgerouter-e300)
  DEVICE_MODEL := EdgeRouter 4
  DEVICE_DTS := cn7130_ubnt_edgerouter-4
endef
TARGET_DEVICES += ubnt_edgerouter-4

define Device/ubnt_edgerouter-6p
  $(Device/ubnt_edgerouter-e300)
  DEVICE_MODEL := EdgeRouter 6P
  DEVICE_DTS := cn7130_ubnt_edgerouter-6p
endef
TARGET_DEVICES += ubnt_edgerouter-6p


define Device/pan_pa-220
  $(Device/ubnt_edgerouter-e300)
  DEVICE_VENDOR := Palo Alto Networks
  DEVICE_MODEL := PA-220
  DEVICE_DTS := cn7130_pan_pa-220
  DEVICE_PACKAGES += luci fdisk e2fsprogs htop mdio-tools kmod-hwmon-tmp421 kmod-fs-cifs cifsmount rsync \
	kmod-wireguard wireguard-tools luci-proto-wireguard \
	strongswan strongswan-mod-openssl strongswan-default luci-app-strongswan-swanctl kmod-crypto-sha1 \
	tune2fs dumpe2fs block-mount kmod-rtc-ds1307 sfdisk
  # The leading "-" replaces U-Boot's autoboot arguments (PanOS init=,
  # 9600-baud console, mem=4096 ...). eMMC layout: p1 = boot (U-Boot loads
  # /vmlinux.oct3-mp from it), p2 = OpenWrt root. PCIe has nothing
  # attached on the PA-220; skip the failing BIST/link probe.
  CMDLINE := -root=/dev/mmcblk0p2 rootfstype=ext4 rootwait console=ttyS0,115200 pcie_octeon.pcie_disable=1
  # Upgrade image = *-targz-sysupgrade.tar (kernel + root .tar.gz), see
  # lib/upgrade/platform.sh. Metadata lets sysupgrade/LuCI check the board.
  IMAGE/sysupgrade.tar := sysupgrade-tar | append-metadata
  # 2.0 = eMMC layout p1 boot / p2 root (created by pa220-install).
  DEVICE_COMPAT_VERSION := 2.0
  DEVICE_COMPAT_MESSAGE := Needs the PA-220 eMMC layout (p1 boot, p2 root). Install with pa220-install from the RAM image. Do not force this upgrade.
endef
TARGET_DEVICES += pan_pa-220

ERLITE_CMDLINE:=-mtdparts=phys_mapped_flash:512k(boot0)ro,512k(boot1)ro,64k(eeprom)ro root=/dev/sda2 rootfstype=squashfs,ext4 rootwait
define Device/ubnt_edgerouter-lite
  DEVICE_VENDOR := Ubiquiti
  DEVICE_MODEL := EdgeRouter Lite
  DEVICE_DTS := cn5020_ubnt_edgerouter-lite
  KERNEL := kernel-bin | patch-cmdline | append-dtb-to-elf
  KERNEL_DEPENDS := $$(wildcard $(DTS_DIR)/$(DEVICE_DTS).dts)
  CMDLINE := $(ERLITE_CMDLINE)
  SUPPORTED_DEVICES += erlite ubnt,erlite
endef
TARGET_DEVICES += ubnt_edgerouter-lite

define Device/ubnt_unifi-usg
  DEVICE_VENDOR := Ubiquiti
  DEVICE_MODEL := UniFi Security Gateway
  BOARD_NAME := ubnt,usg
  DEVICE_PACKAGES += kmod-gpio-button-hotplug kmod-leds-gpio
  DEVICE_DTS := cn5020_ubnt_usg
  CMDLINE := $(ERLITE_CMDLINE)
  KERNEL += | append-dtb-to-elf
  SUPPORTED_DEVICES += ubnt,usg
endef
TARGET_DEVICES += ubnt_unifi-usg

define Device/cisco_vedge1000
  DEVICE_VENDOR := Cisco Viptela
  DEVICE_MODEL := vEdge 1000
  BOARD_NAME := cisco,vedge1000
  DEVICE_PACKAGES += \
	blkid \
	kmod-hwmon-jc42 \
	kmod-hwmon-max6697 \
	kmod-of-mdio \
	kmod-rtc-ds1307 \
	kmod-usb-dwc3 \
	kmod-usb-storage-uas \
	kmod-usb3 \
	sfdisk \
	uboot-envtools
  KERNEL := kernel-bin | append-dtb-elf
  KERNEL_DEPENDS := $$(wildcard $(DTS_DIR)/$(DEVICE_DTS).dts)
  DEVICE_DTS := cn6130_cisco_vedge1000
endef
TARGET_DEVICES += cisco_vedge1000

$(eval $(call BuildImage))

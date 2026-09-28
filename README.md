# OpenWrt for the Palo Alto Networks PA-220

A port of OpenWrt 25.12.4 (kernel 6.12) to the Palo Alto Networks PA-220
firewall. OpenWrt runs from the internal eMMC. It is started by our own U-Boot,
installed next to Palo Alto's bootloaders in SPI flash.

**Hardware:** Cavium OCTEON III CN7130 (4 × MIPS64 at 1 GHz), 8 GB DDR,
27.6 GB eMMC, 8 × gigabit front ports (88E1680 over QSGMII) plus an MGT port
(88E1512 over RGMII).

## Features

- **All 9 network ports** work: `lan1`–`lan8` and `mgmt`. MAC addresses are
  read from the board EEPROM.
- **Bridge fast path in the driver.** Known unicast frames between bridge ports
  are forwarded without building an skb. 802.1Q VLAN filtering is supported.
  Gigabit line rate uses about 12 % softirq (about 25–30 % without the fast
  path).
- **Hardware crypto.** AES and GHASH use the OCTEON COP2 unit.
- **LEDs:**
  - The front LEDs (STAT, HA, ALM, TEMP) show boot, running, upgrade and
    kernel-panic states.
  - The port jack LEDs are configurable.
  - There is a night-time "LEDs off" schedule.
- **Sensors.** Temperatures come from the TMP421 and are shown in LuCI. TEMP
  turns orange when the board is hot. The DS1338 RTC is synced from NTP.
- **LuCI pages:**
  - PA-220 LED settings.
  - Temperatures.
  - Network performance, with fast path and RX tuning settings and live
    counters.
- **sysupgrade** from LuCI or the CLI, keeping settings. The kernel's previous
  version is kept as a fallback.
- **Hardware watchdog** (octeon-wdt) with the correct timeout.
- **Our own U-Boot** (Cavium SDK U-Boot 2013.07):
  - Boots OpenWrt from the eMMC in about 0.3 s.
  - Falls back to the previous kernel.
  - Can boot a RAM image over TFTP.
  - Controls the CPLD (LEDs, flash select, watchdog pulse).
  - Palo Alto's own bootloaders stay in SPI flash untouched, as a fallback.

## Default network on first boot

| Interface | Ports | Address |
|---|---|---|
| `lan` (br-lan) | lan1–lan8 | 192.168.1.2/24, gateway 192.168.1.1, DNS 1.1.1.1 |
| `mgmt` | MGT | 192.168.2.1/24, DHCP server on |

## Repository layout

| Path | Contents |
|---|---|
| `openwrt/` | PA-220 files at their OpenWrt paths, `pa220.diffconfig`, and `setup.sh` (clones v25.12.4 and adds the files) |
| `u-boot/` | Full U-Boot source tree, with `build.sh` |
| `releases/` | Prebuilt images: initramfs (RAM boot), targz sysupgrade image, U-Boot, and `sha256sums` |
| `docs/` | Documentation |
| `LICENSES/` | License texts |

## Building

```sh
# OpenWrt
openwrt/setup.sh <build-dir>        # clone, add PA-220 files, feeds, defconfig
cd <build-dir> && make -j$(nproc)

# U-Boot (needs the Cavium SDK tools-gcc-4.7 toolchain)
u-boot/build.sh <path-to>/tools-gcc-4.7
```

## Warning

Installing this replaces PAN-OS on the eMMC and writes a bootloader into SPI
flash. Back up the eMMC and both SPI flash chips first. Never write the
FAILSAFE SPI chip. You use this at your own risk.

## License

- OpenWrt and kernel parts: GPL-2.0.
- U-Boot: GPL-2.0+; the Cavium OCTEON SDK files (`cvmx-*`) are under the
  Cavium BSD-style license.

See `LICENSES/`.

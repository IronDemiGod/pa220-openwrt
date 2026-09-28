# PA-220 OpenWrt: Features

This page describes what the port supports and how each part works. It
covers the hardware support, the network defaults, performance, LEDs and
sensors, the LuCI pages, upgrades and our U-Boot.

- [Hardware support](#hardware-support)
- [Network](#network)
- [Performance](#performance)
- [LEDs](#leds)
- [Temperatures and clock](#temperatures-and-clock)
- [LuCI pages](#luci-pages)
- [Installation, upgrades and recovery](#installation-upgrades-and-recovery)
- [U-Boot](#u-boot)
- [Included packages](#included-packages)
- [Other defaults](#other-defaults)
- [Not supported / untested](#not-supported--untested)

---

## Hardware support

| Component | Part | Status |
|---|---|---|
| SoC | Cavium OCTEON III CN7130, 4 × MIPS64r2 at 1 GHz, big-endian | All 4 cores used |
| RAM | 8 GB DDR | Fully usable |
| Storage | 27.6 GB eMMC (Micron) | Boot and root file system |
| Front ports 1–8 | Marvell 88E1680 octal PHY over two QSGMII links | `lan1`–`lan8`, gigabit |
| MGT port | Marvell 88E1512 over RGMII | `mgmt`, gigabit |
| MAC addresses | Board EEPROM | lan1–lan8 = base + 0…7, mgmt = base + 8 |
| Hardware crypto | OCTEON COP2 unit | AES (ECB/CBC/CTR) and GHASH drivers |
| Temperature sensor | TMP421 | Board and CPU die temperatures |
| Real-time clock | DS1338 | Kept in sync from NTP |
| Front LEDs | Driven through the board CPLD | STAT, HA, ALM, TEMP (PWR is hardwired) |
| Port LEDs | 88E1680 LED matrix | Configurable |
| Watchdog | octeon-wdt | Active, correct 30 s timeout |
| SPI flash | 2 × 16 MB (normal and failsafe) | Read-only in Linux |
| Serial console | ttyS0 | 115200 baud |

Kernel patches for the board:
- The board fixups take the port PHY out of reset, power it up, set up its
  LED matrix and read the MAC addresses from the EEPROM.
- The CN70xx AGL interface is supported, which makes the MGT port work.
- A core-mask fallback makes all 4 cores come up.
- The eMMC works in SDR mode, which is reliable on this board.
- The watchdog driver reports its real timeout, so procd no longer reboots
  the box after a short stall.

## Network

### Default configuration (first boot)

| Interface | Ports | Settings |
|---|---|---|
| `lan` (`br-lan`) | lan1–lan8 in one bridge | 192.168.1.2/24, gateway 192.168.1.1, DNS 1.1.1.1, no DHCP server |
| `mgmt` | MGT | 192.168.2.1/24, DHCP server with addresses .100–.149 |

**The front ports** are set up to sit behind an existing router at
192.168.1.1. That router keeps handing out addresses, so the PA-220 doesn't
start a second DHCP server on the LAN.

**The MGT port** is its own management network. Plug a PC straight into it
and it gets an address. You can then reach LuCI and SSH at 192.168.2.1.

MGT has its own firewall zone, `mgmt`. It accepts traffic to the router
itself (ping, SSH, LuCI) but never forwards traffic to or from other
networks.

These defaults are applied only to a fresh configuration. Settings kept
over a sysupgrade or restored from a backup are left alone.

### No switch chip

The PA-220 has no switch chip, so the CPU handles every frame between the
front ports. Each port is a normal Linux network interface. You can bridge
them, route between them, use VLANs, or set one up as a WAN port, just as
with any other interface.

The unplugged-MGT-port problem is fixed: by default a static route on an
unplugged MGT port no longer takes over replies meant for the same subnet
on `br-lan`.

## Performance

### Bridge fast path

The driver has its own fast path for bridged traffic. It is on by default.

**How it works:**
- A frame going to a known device on another port of the same bridge is
  sent straight back out by the driver. It skips the Linux network stack.
- The driver keeps its own MAC address table: 4096 entries, with a 300 s
  age-out.
- Once a minute, one frame per host is passed to the Linux bridge, so the
  bridge's own table stays current.

**Linux still handles:**
- broadcast and multicast frames;
- frames to unknown destinations;
- traffic addressed to the PA-220 itself;
- routed traffic.

**VLANs:** 802.1Q VLAN-filtering bridges are supported. Tags are added or
removed per port as configured. This part is compile-tested only, not yet
tested on hardware.

**The fast path turns itself off** for a port when:
- the bridge uses 802.1ad or MST;
- the port is blocked by STP, isolated or locked;
- bridge firewall rules (nftables bridge family or ebtables) exist;
- a packet capture such as `tcpdump` is running.

**Limitation:** per-port traffic shaping and bridge statistics don't see
fast-path frames.

**Measured on a bridged gigabit internet connection:**

| | Download | Upload |
|---|---|---|
| Fast path on | 945 Mbit/s at about 12 % softirq | 470 Mbit/s at about 6 % softirq |
| Fast path off | 940 Mbit/s at about 25–30 % softirq | — |

The upload figure is what the internet connection delivered, not a limit of
the PA-220. The softirq figure is the share of CPU time, over all four
cores, spent processing packets in the kernel.

### Receive path

- Received packets are spread over all four cores in hardware, using four
  receive groups.
- There is a larger buffer pool.
- Receive checksums are checked in hardware.
- Tunable at run time, without a reboot:
  - **GRO**: on by default.
  - **Receive interrupt delay**: 1–15 ticks, default 1.
  - **Interrupt after N packets**: 0–255, default 0 (off).
- **Packet steering (RPS) is off by default.** The hardware already spreads
  the load, so RPS only added work: 35–40 % softirq with it, compared with
  25–30 % without.

### Routed traffic

Firewall software flow offloading is on by default. Established routed or
NATed connections then skip most of the firewall path. You can change it in
LuCI under Network → Firewall → General Settings.

### Crypto

The COP2 AES and GHASH drivers speed up AES-CBC, AES-CTR and AES-GCM for
IPsec (strongSwan) and anything else that uses the kernel crypto API.
WireGuard doesn't use AES, so it doesn't benefit. The crypto self-tests are skipped at boot.

## LEDs

### Front panel

| LED | Meaning |
|---|---|
| STAT orange | Booting (solid); blinking = failsafe mode or firmware upgrade |
| STAT green | Running |
| ALM red | Kernel panic |
| TEMP green / orange | Temperature normal / above the "hot" threshold |
| HA green | Night-mode indicator (optional) |
| PWR | Hardwired, always on |

### Port (jack) LEDs

Each jack has two LEDs, and you can set the mode of each one:
- **Left LED** (default: on while there is a link). Other modes: link plus
  blink on activity, blink count for speed, activity, transmit, gigabit link
  only, always on, always off, always blinking.
- **Right LED** (default: on at 100 or 1000 Mbit/s). Other modes: link plus
  activity, link plus receive, activity, 100 Mbit/s only, always on, always
  off, always blinking.

The hardware has no "gigabit only" mode for the right LED.

### LED control

- **Jack LEDs** and **front-panel LEDs** can each be switched on or off.
- **Night mode:** all LEDs off between two times, for example 23:00–07:00.
  The period may run past midnight. Optionally, HA stays solid green during
  the night period as an indicator.
- ALM red always keeps its kernel-panic function, even when the LEDs are
  switched off.
- The settings are in `/etc/config/pa220`, section `leds`.
- Command line:
  `pa220-leds on | off | status | apply`.
- A small background service, `/etc/init.d/pa220-leds`, applies the
  settings and the night schedule. It checks every 30 s.

## Temperatures and clock

**Temperatures:** the TMP421 reports the CPU die temperature (about 72 °C
under load) and the board temperature.

**TEMP LED:** turns orange at the "hot" threshold (default 85 °C) and green
again below the "cool" threshold (default 80 °C). The thresholds are set in
LuCI and read again every 15 s.

**RTC:** the DS1338 is written every time NTP sets or synchronises the
clock. The time is therefore correct at the next boot even after a power
cut. U-Boot's `date` command shows it too.

## LuCI pages

These are added without changing any LuCI package files.

| Menu | Page | Contents |
|---|---|---|
| Status | **Temperatures** | Live readings for each sensor, with status (ok / hot / sensor fault); refresh interval 2 s – 1 min; TEMP LED thresholds |
| System → LED Configuration | **PA-220** tab (next to the standard **LEDs** tab) | Jack LEDs on/off and modes, front LEDs on/off, night mode times, HA night indicator |
| Network | **PA-220 Performance** | Fast path on/off; GRO, interrupt delay and packet count; packet steering; live status |

The live status on the Performance page shows:
- whether the fast path is on;
- the ports it is active on;
- frames forwarded by the fast path;
- frames passed to the bridge;
- unknown destinations;
- times the hardware queue was full;
- frames skipped while a capture was running;
- softirq CPU load.

The same data is available on the command line with `pa220-net status`.

Changes on the Performance page take effect immediately, without a reboot.

## Installation, upgrades and recovery

### eMMC layout

| Partition | Size | Contents |
|---|---|---|
| p1 | 256 MiB ext3 | Kernel `/vmlinux.oct3-mp` and backup `/vmlinux.oct3-mp.bak`; mounted at `/boot` |
| p2 | Rest of the eMMC (about 27 GiB) | ext4 root file system |

### Images

| File | Use |
|---|---|
| `…-initramfs-kernel.bin` | RAM image, booted over TFTP from U-Boot (`run linux_ram`). Root password `password`. |
| `…-targz-sysupgrade.tar` | Installation and upgrade image |

### Installing

`pa220-install <tftp-server> [image] [backup|none]` runs on the serial
console of the RAM image. It:
1. fetches the image and, optionally, a LuCI settings backup over TFTP;
2. partitions and formats the whole eMMC (it asks for "yes" first);
3. installs the image;
4. restores the settings on first boot.

The same command re-installs a broken system.

### Upgrading

**Standard sysupgrade:** from LuCI (System → Backup / Flash Firmware) or
with `sysupgrade -v <image>`. Settings are kept.

What happens during an upgrade:
- The new kernel is written to p1, and the old one is kept as
  `.bak`. U-Boot boots the `.bak` kernel automatically if the new one
  can't be loaded.
- The root file system is recreated and your settings are restored.
- The image carries compatibility version 2.0, so older firmware with the
  previous eMMC layout refuses it instead of breaking the system.
- Extra packages installed with `apk` are not kept over an upgrade.

## U-Boot

This is our own build of the Cavium SDK U-Boot 2013.07, stored in the
normal SPI flash chip at 0x250000.

**Palo Alto's bootloaders are untouched.** The boot chain is:
1. Palo Alto's first stages set up the DRAM, then start our U-Boot.
2. If ours is corrupt, they start Palo Alto's own U-Boot instead.
3. If ours hangs, the CPLD switches to the failsafe SPI chip, which we
   never write.

**What it does:**
- Boots `/vmlinux.oct3-mp` from the eMMC, falling back to `.bak`. The
  2-second countdown can be interrupted with a key.
- Loads the kernel in about 0.3 s: the eMMC runs in 8-bit DDR mode, and a
  100 ms wait on every read was removed.
- `run linux_ram` boots the RAM image over TFTP.
- Reads the MAC addresses and serial number from the board EEPROM and
  passes them to Linux.
- Configures the QSGMII links for the front ports.
- Sets up the CPLD:
  - maps it for Linux;
  - drives the status LEDs (STAT orange at start, ALM green while in the
    shell);
  - sends the "boot OK" pulse that stops the failsafe switch-over.
- Holds the port PHY in reset until Linux takes over, so the port LEDs
  stay dark during boot.
- Has its own device tree.
- Saves its environment to SPI flash with `saveenv`.
- Extra commands: `cpld`, `date` (RTC), `usb`, `ext4write`, `fatwrite`,
  `hash`, `mtest` and more.

## Included packages

On top of the standard OpenWrt set, the image includes:

- **Web interface:** LuCI.
- **VPN:**
  - WireGuard, with its LuCI support;
  - strongSwan (IPsec), with its LuCI app.
- **Storage and file systems:** `fdisk`, `sfdisk`, `e2fsprogs`, `tune2fs`,
  `dumpe2fs`, `block-mount`, CIFS/SMB mounting.
- **Tools:** `htop`, `rsync`, `mdio-tools`.
- **Drivers:**
  - TMP421 (temperatures);
  - DS1307/DS1338 (RTC).

## Other defaults

- **Kernel messages** are kept off the serial console once the system is
  up, so they don't interrupt typing. They are still in `dmesg`. You can
  change this in LuCI under System → System → Logging.
- **Boot:**
  - the OpenWrt failsafe wait is 2 s instead of the usual 4 s;
  - the 1 s wait for a non-existent wireless configuration is skipped.
- **The RAM image** gets the root password `password`, so it never runs
  with an empty password. Installed systems are not affected.

## Not supported / untested

- **USB:** both xHCI controllers are enabled but not yet tested with a
  device.
- **PCIe:** nothing is attached on the PA-220, so it is disabled.
- **VLAN fast path:** compile-tested only.
- **Routed fast path:** routed traffic uses the standard kernel path with
  flow offloading, not the driver fast path.

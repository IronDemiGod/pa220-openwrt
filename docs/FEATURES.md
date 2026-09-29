# PA-220 OpenWrt Features

- [Hardware support](#hardware-support)
- [Network](#network)
- [Performance](#performance)
- [LEDs](#leds)
- [Temperatures and RTC](#temperatures-and-rtc)
- [LuCI pages](#luci-pages)
- [eMMC layout](#emmc-layout)
- [U-Boot](#u-boot)
- [Included packages](#included-packages)
- [Other defaults](#other-defaults)
- [Not supported / untested](#not-supported--untested)

---

## Hardware support

| Component | Part | Status |
|---|---|---|
| SoC | Cavium OCTEON III CN7130, 4 × MIPS64r2 @ 1 GHz, big-endian | All 4 cores |
| RAM | 8 GB DDR | Full 8 GB |
| Storage | 27.6 GB eMMC (Micron) | Boot + root |
| Ports 1–8 | Marvell 88E1680 octal PHY, 2 × QSGMII | `eth1`–`eth8`, gigabit |
| MGT port | Marvell 88E1512, RGMII | `mgt`, gigabit |
| MAC addresses | Board EEPROM | eth1–8 = base+0…7, mgt = base+8 |
| Crypto | OCTEON COP2 | AES (ECB/CBC/CTR), GHASH |
| Temp sensor | TMP421 | CPU die + board |
| RTC | DS1338 | Synced from NTP |
| Front LEDs | via CPLD | STAT, HA, ALM, TEMP (PWR hardwired) |
| Port LEDs | 88E1680 LED matrix | Configurable |
| Watchdog | octeon-wdt | Working, 30 s timeout |
| SPI flash | 2 × 16 MB (normal + failsafe) | Read-only in Linux |
| Serial console | ttyS0 | 115200 baud |

Board kernel patches:
- Port PHY: out of reset, powered up, LED matrix set up.
- MACs read from the EEPROM.
- CN70xx AGL support (MGT port).
- Core-mask fallback (all 4 cores come up).
- eMMC in SDR mode (stable on this board, DDR was very unstable).
- Watchdog reports the real timeout (no more reboot after a short stall).

## Network

### First-boot defaults

| Interface | Ports | Settings |
|---|---|---|
| `lan` (`br-lan`) | eth1–eth8 | 192.168.1.2/24, gateway 192.168.1.1, DNS 1.1.1.1, DHCP server off |
| `mgt` | MGT (`mgt`) | 192.168.2.1/24, DHCP server on (.100–.149) |

- Front ports: All bridged (basically a switch).
- MGT: separate management network. Plug a PC in directly, LuCI/SSH at
  192.168.2.1.
- MGT firewall zone `mgt`: ping/SSH/LuCI to the box allowed, no forwarding
  to/from other networks.
- Defaults only apply to a fresh config. Kept/restored settings are left
  alone.

### No switch chip

- All traffic between the front ports goes through the CPU.
- Every port is a normal Linux interface: bridge, route, VLAN, WAN, etc.

## Performance

### Bridge fast path (on by default)

- Frames to a known device on another port of the same bridge are sent
  straight back out by the driver, skipping the Linux network stack.
- Own MAC table: 4096 entries, 300 s age-out.
- One frame per host per minute still goes to the Linux bridge (keeps its
  table fresh).
- Still handled by Linux: broadcast, multicast, unknown destinations,
  traffic to the box itself, routed traffic.
- 802.1Q VLAN-filtering bridges supported, tags added/stripped per port
  (compile-tested only, not tested on hardware yet).
- Turns itself off for a port when:
  - bridge uses 802.1ad or MST
  - port is STP-blocked, isolated or locked
  - bridge firewall rules exist (nftables bridge / ebtables)
  - a packet capture is running (`tcpdump`)
- Limitation: per-port traffic shaping and bridge statistics don't see
  fast-path frames.

Measured (bridged gigabit internet):

| | Download | Upload |
|---|---|---|
| Fast path on | 945 Mbit/s, ~12 % softirq | 910 Mbit/s, ~6 % softirq |
| Fast path off | 940 Mbit/s, ~25–30 % softirq | 850 Mbit/s, ~15–20 % softirq |

- UL/DL speeds were limited by my ISP, not the firewall.
- softirq % = share of CPU time (all 4 cores) spent processing packets in
  the kernel.

### Receive path

- Received packets spread over all 4 cores in hardware (4 receive groups).
- Larger buffer pool.
- Hardware receive checksum.
- Tunable live (no reboot):
  - GRO: on by default
  - receive interrupt delay: 1–15 ticks, default 1
  - interrupt after N packets: 0–255, default 0 (off)
- Packet steering (RPS) off by default. Hardware already spreads the load,
  RPS only added work (35–40 % softirq on vs 25–30 % off).

### Routed traffic

- Firewall software flow offloading on by default (established routed/NAT
  connections skip most of the firewall path).

### Crypto

- COP2 AES and GHASH drivers: faster AES-CBC, AES-CTR and AES-GCM for IPsec
  (strongSwan) and anything else using the kernel crypto API.
- WireGuard doesn't use AES, so no gain there.

## LEDs

### Front panel

| LED | Meaning |
|---|---|
| STAT orange | Solid = booting, blinking = failsafe mode or firmware upgrade |
| STAT green | Running |
| ALM red | Kernel panic |
| TEMP green / orange | Normal / above the "hot" threshold |
| HA green | Night mode indicator (optional) |
| PWR | Hardwired, always on |

### Port (jack) LEDs

Two LEDs per jack, mode set separately:
- Left (default: on with link). Other modes: link + blink on activity, blink
  count = speed, activity, transmit, gigabit link only, always on, always
  off, always blinking.
- Right (default: on at 100/1000 Mbit/s). Other modes: link + activity,
  link + receive, activity, 100 Mbit/s only, always on, always off, always
  blinking.
- No "gigabit only" mode for the right LED (hardware limit).

### LED control

- Jack LEDs and front LEDs: on/off separately.
- Night mode: all LEDs off between two times (e.g. 23:00–07:00, can wrap
  past midnight). Optional HA solid green during the night period.
- ALM red keeps its kernel-panic function even with the LEDs off.

## Temperatures and RTC

- TMP421: CPU die (~72 °C under load) and board temperature.
- TEMP LED: orange at "hot" (default 85 °C), green again below "cool"
  (default 80 °C). Thresholds set in LuCI, re-read every 15 s.
- DS1338 RTC: written every time NTP sets/syncs the clock, so the time is
  right at the next boot even after a power cut. Also shown by U-Boot
  `date`.

## LuCI pages

Added without changing any LuCI package files.

| Menu | Page | Contents |
|---|---|---|
| Status | **Temperatures** | Live readings per sensor with status (ok / hot / sensor fault), refresh interval 2 s – 1 min, TEMP LED thresholds |
| System → LED Configuration | **PA-220** tab (next to the stock **LEDs** tab) | Jack LEDs on/off + modes, front LEDs on/off, night mode times, HA night indicator |
| Network | **PA-220 Performance** | Fast path on/off, GRO, interrupt delay/packets, packet steering, live status |

Performance page live status:
- fast path on/off
- ports it's active on
- frames forwarded by the fast path
- frames passed to the bridge
- unknown destinations
- hardware queue full
- frames skipped during a capture
- softirq CPU load

- Same data on the CLI: `pa220-net status`.
- Changes apply instantly, no reboot.

## eMMC layout

| Partition | Size | Contents |
|---|---|---|
| p1 | 256 MiB ext3 | Kernel `/vmlinux.oct3-mp` + backup `/vmlinux.oct3-mp.bak`, mounted at `/boot` |
| p2 | Rest (~27 GiB) | ext4 root |

(p1 is ext3: the format the old SDK U-Boot `ext2load` is proven to read.)

## U-Boot

Custom build of Cavium SDK U-Boot 2013.07, in the normal SPI flash chip at
0x250000.

Boot chain (Palo Alto's bootloaders untouched):
1. Palo Alto's early stages set up DRAM, then start our U-Boot.
2. Ours corrupt → Palo Alto's own U-Boot starts instead.
3. Ours hangs → CPLD switches to the failsafe SPI chip (never written).

Features:
- Boots `/vmlinux.oct3-mp` from eMMC, falls back to `.bak`. 2 s countdown,
  any key stops it.
- Kernel load ~0.3 s (eMMC 8-bit DDR, 100 ms wait per read removed).
- TFTP boot of the RAM image.
- MACs + serial number from the board EEPROM, passed to Linux.
- QSGMII links set up for the front ports.
- CPLD:
  - mapped for Linux
  - status LEDs (STAT orange at start, ALM green in the shell)
  - "boot OK" pulse (stops the failsafe switch-over)
- Port PHY held in reset until Linux takes over (port LEDs dark during
  boot).
- Own device tree.
- `saveenv` to SPI flash.
- Extra commands: `cpld`, `date` (RTC), `usb`, `ext4write`, `fatwrite`,
  `hash`, `mtest`, and more.

## Included packages

On top of the standard OpenWrt set:
- Web UI: LuCI
- VPN:
  - WireGuard + LuCI support
  - strongSwan (IPsec) + LuCI app
- Storage: `fdisk`, `sfdisk`, `e2fsprogs`, `tune2fs`, `dumpe2fs`,
  `block-mount`, CIFS/SMB mounting
- Tools: `htop`, `rsync`, `mdio-tools`
- Drivers:
  - TMP421 (temperatures)
  - DS1307/DS1338 (RTC)

## Other defaults

- Kernel messages off the serial console once booted (still in `dmesg`).
  Setting: LuCI → System → System → Logging
- RAM image root password `password` (never runs with an empty password).
  Installed systems not affected.

## Not supported / untested

- USB: both xHCI controllers enabled, not tested with a device yet.
- PCIe: nothing attached on the PA-220, disabled.
- VLAN fast path: compile-tested only.
- Routed fast path: none. Routed traffic uses the normal kernel path +
  flow offloading.

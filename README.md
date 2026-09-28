# OpenWrt for the PA-220 


<img width="414" height="149" alt="PA-220" src="https://github.com/user-attachments/assets/60f61e89-6e2f-4d70-82cb-9b33070301cf" />


A port of OpenWrt to the Palo Alto Networks PA-220
firewall. OpenWrt runs from the internal eMMC, Initialised by a custom U-Boot build
installed next to Palo Alto's bootloaders in SPI flash.

## PA-220 Hardware Overview
 
Cavium OCTEON III CN7130 (4 × MIPS64 at 1 GHz), 8 GB DDR,
27.6 GB eMMC, 8 × gigabit ports + MGT port

## Features

- **9 network ports** work: `lan1`–`lan8` and `mgmt`. MAC addresses are
  read from the board EEPROM.
- **Bridge fast path in the driver.** Known unicast frames between bridge ports
  are forwarded without building an skb. 802.1Q VLAN filtering is supported.
  Gigabit line rate uses about 12 % softirq (it is 25–30 % without fast
  path). (softirq % is the share of CPU time used to process packets in the kernel).
- **Hardware crypto.** AES and GHASH use the OCTEON COP2 unit.
- **LEDs:**
  - The front LEDs (STAT, HA, ALM, TEMP) show boot, running, upgrade and
    kernel-panic states, and are configurable in LuCI.
  - The port jack LEDs are configurable in LuCI
- **Sensors:** CPU temperatures are shown in LuCI. TEMP
  LED thresholds can be configured in LuCI.
- **PA-220-specific LuCI pages/sections:**
  - PA-220 LED settings.
  - Temperatures.
  - Network performance, (fastpath, RX settings, statistics)
- **Boot time:** 28 seconds. 
- **Hardware watchdog** (octeon-wdt).
- **Custom U-Boot** (Cavium SDK U-Boot 2013.07):
  - Kernel load times are way faster than the original U-boot.
  - Backup kernel fallback.
  - TFTP Boot.
  - CPLD control (CPLD controls LEDs, flash select, watchdog pulse).
  - Palo Alto's initial stage bootloaders are untouched and are what load the custom U-boot.
  - Second SPI flash chip, which is a failsafe, remains untouched.
- **RAM and storage:** 27.6 GB eMMC storage, 8 GB DDR memory.

- [Full Features List](docs/FEATURES.md)

## Installation
- [Installation Guide](docs/INSTALLATION.md)
- **The installation procedure is very lengthy**

## Upgrading OpenWrt
- [Upgrading Guide](docs/UPGRADING.md)

## Recovery
- [Recovery Suggestions](docs/RECOVERY.md)

## License

- OpenWrt and kernel parts: GPL-2.0.
- U-Boot: GPL-2.0+; the Cavium OCTEON SDK files (`cvmx-*`) are under the
  Cavium BSD-style license.

See `LICENSES/`.

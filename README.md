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
  Gigabit line rate uses about 12 % softirq (about 25–30 % without the fast
  path).
- **Hardware crypto.** AES and GHASH use the OCTEON COP2 unit.
- **LEDs:**
  - The front LEDs (STAT, HA, ALM, TEMP) show boot, running, upgrade and
    kernel-panic states, and are configurable in LuCI.
  - The port jack LEDs are configurable in LuCI
- **Sensors.** CPU temperatures are shown in LuCI. TEMP
  turns orange when the board is hot.
- **PA-220-specific LuCI pages/sections:**
  - PA-220 LED settings.
  - Temperatures.
  - Network performance, (fastpath, RX settings, statistics)
- **Hardware watchdog** (octeon-wdt) with the correct timeout.
- **Custom U-Boot** (Cavium SDK U-Boot 2013.07):
  - Boots OpenWrt from the eMMC in about 0.3 s.
  - Falls back to the previous kernel.
  - Can boot a RAM image over TFTP.
  - Controls the CPLD (LEDs, flash select, watchdog pulse).
  - Palo Alto's initial stage bootloaders are untouched and are what load the custom U-boot

## License

- OpenWrt and kernel parts: GPL-2.0.
- U-Boot: GPL-2.0+; the Cavium OCTEON SDK files (`cvmx-*`) are under the
  Cavium BSD-style license.

See `LICENSES/`.

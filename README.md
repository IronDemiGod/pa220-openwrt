# OpenWrt for the PA-220 


<img width="414" height="149" alt="PA-220" src="https://github.com/user-attachments/assets/60f61e89-6e2f-4d70-82cb-9b33070301cf" />
<img width="200" height="200" alt="image" src="https://github.com/user-attachments/assets/233336cb-f35a-451b-bbc8-418e97c6e135" />


A port of OpenWrt to the Palo Alto Networks PA-220
firewall. OpenWrt runs from the internal eMMC, Initialised by a custom U-Boot build
installed next to Palo Alto's bootloaders in SPI flash.

This makes the PA-220 a fully capable Gigabit router/firewall, and more.

---

## Features

- **9 network ports** work: `lan1`–`lan8` and `mgmt`. MAC addresses are
  read from the board EEPROM.
- **Hardware crypto.** AES and GHASH use the OCTEON COP2 unit.
- **Bridged fast-path** (check full feature list)
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
- **Custom U-Boot** (Cavium SDK U-Boot 2013.07):
  - Faster Kernel load times. (way faster), Backup kernel fallback. TFTP boot, LED control.
- **RAM and storage:** 27.6 GB eMMC storage, 8 GB DDR memory.

- [Full Features List](docs/FEATURES.md)

---
## Photos and Screenshots:

### IRL Look

<img width="853" height="273" alt="output" src="https://github.com/user-attachments/assets/4b3a311f-ac92-47fa-8010-28c3b3ea2224" />


<table>
  <tr>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/4fbb7df8-2f00-4da2-b84c-ca3ec1f29c1e" width="400" alt=""><br>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/d2b9fc08-e92a-4d92-9f5f-692883a3608f" width="400" alt=""><br>
    </td>
  </tr>
  <tr>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/3e911ee8-40bf-4944-ac70-b5e577c1dbcf" width="400" alt=""><br>
    </td>
    <td align="center">
      <img src="https://github.com/user-attachments/assets/74fa35a6-cb6f-4378-805a-4a8171ec06ee" width="400" alt=""><br>
    </td>
  </tr>
</table>

---

## Installation
- [Installation Guide](docs/INSTALLATION.md)
- **The installation procedure is very lengthy**

### Upgrading OpenWrt
- [Upgrading Guide](docs/UPGRADING.md)

### Recovery
- [Recovery Suggestions](docs/RECOVERY.md)

---

## License

- OpenWrt and kernel parts: GPL-2.0.
- U-Boot: GPL-2.0+; the Cavium OCTEON SDK files (`cvmx-*`) are under the
  Cavium BSD-style license.

See `LICENSES/`.

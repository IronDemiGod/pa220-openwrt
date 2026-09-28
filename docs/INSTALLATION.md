# Installation Steps

## Pre-requisites

- PA-220 running PanOS (any version).
- Console cable: RJ45 console → USB serial adapter cable, or the micro-USB port.
- Terminal program (PuTTY, Tera Term, minicom, …): **9600 baud, 8N1, no
  flow control**.


## Part 1: Obtaining U-boot shell access

### General outline of the procedure:

This is a very lengthy procedure, unlike many other openwrt-compatible devices. It involves breaking U-boot to get into the shell, rewriting part of the bootloader, and wiping PanOS from internal storage.
Full backups of the SPI flash chips and the emmc can be taken, and therefore can be used to restore the original state of the box via U-boot and a ram-booted linux image (Described later.)

We first need to obtain access to the U-boot prompt. Digging through a dump of the U-boot data revealed that you can type the word "pass" in the screen that asks you to enter 'maint',but it then asks for a password. Claude got flagged when it attempted to find the password. I believe this gives is access to the u-boot shell, and it would've been the easier way had we known the password.

We instead do this by entering the Maintenance menu through the serial console. We then use a 'bootstrapping' tool which re-formats the partition targeted by u-boot when it searches for the kernel. After this, once it fails to find the kernel during boot, it lands in the shell.


### 1. Connect the console

1. Plug the cable into the **CONSOLE** port.
**The CONSOLE port is better, but the micro-usb is also good.**
2. Open the terminal at 9600 8N1.
3. Power on (or reboot) the PA-220.

### 2. Boot into maintenance mode

#### Method 1: Interrupting Boot
1. Watch the boot messages. U-Boot prints:
   ```
   Enter 'maint' to boot to maint partition.
   ```
2. Type `maint` and press **Enter** right away (the window is a few seconds).
3. It can print nothing for a while (slow kernel load). Wait.
4. The maintenance menu comes up (text screen, "Welcome to maintenance
   mode").

#### Method 2: PanOS CLI:
If you are already logged in to the PanOS CLI, this also works:
```
debug system maintenance-mode
```
→ confirm with `y`. The box reboots straight into maintenance mode.

### 3. Open "Disk Image - Advanced"

1. **Continue** (on the welcome screen).
2. Main menu → **Disk Image**.
   - Shows "Currently active version: x.y.z". Note it.
3. **Advanced Options**.
4. Password prompt `Enter password for advanced options:` → type the
   PanOS maintenance-mode password → **Enter**.
   - Not in this repo. Well known: search Google for "Palo Alto maintenance
     mode password".
   - Shown as `*****`. Wrong → "Password incorrect! Try again."
5. Screen title: **Disk Image - Advanced**.

Screen layout (top to bottom):
```
[ List ] [ Status ] [ History ] [ Revert ] [ Cancel ]
------------------------------------------------------
( ) 10.0.6    ( ) 9.0.0    ...          <- images
[ Info ] [ Verify ] [ Purge ]
------------------------------------------------------
( ) sysroot0  ( ) sysroot1  ( ) maint   <- partitions
[ Boot ]
------------------------------------------------------
( ) maint     ( ) panos     ( ) content <- components
[ Refresh ]
------------------------------------------------------
[ Bootstrap ]   <image>, <partition>, <component>
```

### 4. Find the active partition

1. Press **Status**. Output:
   ```
   Partition         State             Version
   ------------------------------------------------
   sysroot0          ACTIVE            10.0.6
   sysroot1          REVERTABLE        9.0.0
   maint             ...
   ```
2. Note the sysroot marked **ACTIVE** (`sysroot0` or `sysroot1`).
   This is the one U-Boot boots from.
3. **Back**.

### 5. Bootstrap the "maint" component into it

On the Disk Image - Advanced screen:

1. Images row: select any image (e.g. the active version) → **Enter**.
2. Partitions row: select the **ACTIVE sysroot** from step 4 → **Enter**.
3. Components row: select **`maint`** → **Enter**.
4. Check the text next to **Bootstrap**. It must read:
   ```
   <image>, <ACTIVE sysroot>, maint
   ```
   e.g. `10.0.6, sysroot0, maint`.
5. Move to **Bootstrap** → **Enter**.
6. Result screen:
   ```
   Command: bootstrap
   Label: sysroot0
   comp: maint
   image: 10.0.6
   Status: Success
   ```

(What it did: formatted the active sysroot (`mkfs.ext3`), created only
`dev/` and `var/log/`. The maint component installs nothing there → no
kernel left.)

### Menu Map of our steps:

```
Maintenance Mode (main menu)
├─ Maintenance Entry Reason
├─ Get System Info
├─ Factory Reset ─ Advanced (password)
├─ FSCK (Disk Check)
├─ Log Files
├─ Bootloader Recovery (password)
├─ Disk Image                              ← 1
│   ├─ Reinstall <active version>
│   ├─ Revert to <previous version>
│   └─ Advanced Options                    ← 2
│       └─ "Enter password for advanced options:"  MA1NT   ← 3
│           └─ Disk Image - Advanced
│               ├─ List | Status | History | Revert | Cancel
│               ├─ ( ) images: 10.0.6, 9.0.0, …   [Info] [Verify] [Purge]
│               ├─ ( ) partitions: sysroot0 / sysroot1 / maint   [Boot]
│               ├─ ( ) components: maint / panos / content   [Refresh]
│               └─ [Bootstrap]  "<image>, <partition>, <component>"   ← 4
├─ Select Running Config
├─ Content Rollback
├─ Set IP Address
├─ Diagnostics (password)
├─ Debug Reboot
└─ Reboot
```

### 6. Reboot into the U-Boot shell

1. On the result screen → **Reboot** (or main menu → **Reboot**).
2. Don't type `maint` this time.
3. U-Boot tries to load the kernel from the empty partition, fails, and
   stops at:
   ```
   Kingfisher(ram) (mp)#
   ```



## Part 2: Flashing the new U-Boot

### General outline of the procedure:


### 7. Files

From the Releases page, or built from this repo (`openwrt/setup.sh`,
`u-boot/build.sh`):

| File | What |
|---|---|
| `u-boot-octeon_pa220.bin` | Our U-Boot |
| `openwrt-octeon-generic-pan_pa-220-initramfs-kernel.bin` | OpenWrt RAM image (installer / recovery) |
| `openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar` | OpenWrt install / upgrade image |

Keep the file names exactly as above (U-Boot and `pa220-install` look for
them).

### 8. Set up the PC

1. TFTP server:
   - Windows: Tftpd64. Linux: `tftpd-hpa`.
   - Put the 3 files in its folder.
   - Windows firewall: allow the TFTP server (UDP 69).
2. Network cable: PC ↔ PA-220 **MGT** port (directly, no switch needed).
3. PC network adapter, static IP:
   - IP `192.168.2.10`
   - Netmask `255.255.255.0`
   - No gateway
4. Tftpd64: "Server interfaces" → `192.168.2.10`.

Addresses used below:

| | IP |
|---|---|
| PC / TFTP server | 192.168.2.10 |
| PA-220 in U-Boot | 192.168.2.2 |
| PA-220 in OpenWrt (MGT) | 192.168.2.1 |

### 9. Start our U-Boot from RAM

At the Palo Alto prompt `Kingfisher(ram) (mp)#` (terminal still 9600).
Type each line, then **Enter**:

```
setenv ipaddr 192.168.2.2
setenv serverip 192.168.2.10
setenv ethact octrgmii0
tftpboot 0x81000000 u-boot-octeon_pa220.bin
```

- Expected: `Bytes transferred = 13xxxxx` (about 1.3 MB).
- `setenv` in Palo Alto's U-Boot is lost at power-off. That's fine.

Then:
```
inv_icache
go 0x81000000
```

1. Switch the terminal to **115200** baud.
2. Press **Enter** a few times. Our prompt: `pa-220#`.
3. If it starts counting down, press any key. (If it runs out, it just fails
   to find a kernel and stops at `pa-220#` anyway.)

Nothing has been written yet. A power cycle brings back Palo Alto's U-Boot.

### 10. Boot OpenWrt from RAM

At `pa-220#`:
```
setenv ipaddr 192.168.2.2
setenv serverip 192.168.2.10
run linux_ram
```

- Loads the initramfs image over TFTP and boots it (~30 s).
- A single "Receive error" / retry during the transfer is normal here.
- Press **Enter** when the boot messages stop → root shell
  `root@OpenWrt:~#`.
- RAM image: root password `password`, MGT = 192.168.2.1.
- Nothing is written to the eMMC or SPI by just booting it.

Check the eMMC and SPI flash are seen:
```
cat /proc/mtd
ls /dev/mmcblk0
```
Expected: an `mtd0` line (the SPI flash, 16 MB = `01000000`) and
`/dev/mmcblk0`.

### 11. Backups (strongly recommended)

Only way back to PanOS later. Run on the **PC**, in **cmd.exe**
(not PowerShell: PowerShell corrupts binary output). Windows 10/11 has
`ssh` built in. Password: `password`. First connection asks to accept the
host key → `yes`.

SPI flash (normal chip, 16 MB, a few seconds):
```
ssh root@192.168.2.1 "cat /dev/mtd0" > pa220-spi-normal.img
```

eMMC (~29.6 GB, takes a while, ~15-30 min):
```
ssh root@192.168.2.1 "cat /dev/mmcblk0" > pa220-mmcblk0.img
```

Check both copies. On the PA-220 console:
```
sha256sum /dev/mtd0 /dev/mmcblk0
```
On the PC (cmd.exe):
```
certutil -hashfile pa220-spi-normal.img SHA256
certutil -hashfile pa220-mmcblk0.img SHA256
```
The hashes must match. Keep both files somewhere safe.

- The failsafe SPI chip is never written by this procedure, no backup
  needed.

### 12. Flash our U-Boot

Back to Palo Alto's U-Boot first. On the PA-220 console:
```
reboot
```

1. Terminal back to **9600** baud.
2. Palo Alto's U-Boot fails to find a kernel → `Kingfisher(ram) (mp)#`.

Start our U-Boot from RAM again. Type each line, then **Enter**:
```
setenv ipaddr 192.168.2.2
setenv serverip 192.168.2.10
setenv ethact octrgmii0
tftpboot 0x81000000 u-boot-octeon_pa220.bin
inv_icache
go 0x81000000
```

Terminal to **115200**, **Enter** → `pa-220#`.

Check the SPI chip select:
```
cpld read d
```
- Must show `Reg d = 0x31` (normal chip). Anything else → **stop**, don't
  flash.
- Never use `cpld write`.

Load the image and flash it:
```
setenv ipaddr 192.168.2.2
setenv serverip 192.168.2.10
tftpboot 0x21000000 u-boot-octeon_pa220.bin
sf probe
sf erase 0x250000 0x14c000
sf write 0x21000000 0x250000 ${filesize}
```

- Only the area 0x250000-0x39bfff is erased/written. Palo Alto's stages
  (0x0-0x7ffff) and U-Boot (0x100000) are not touched.
- Don't type other offsets.

Verify:
```
sf read 0x22000000 0x250000 ${filesize}
cmp.b 0x21000000 0x22000000 ${filesize}
```
Expected: `Total of N byte(s) were the same`. Different → run the
`sf erase` / `sf write` lines again. Don't power off with a failed compare
(if you do anyway, Palo Alto's U-Boot starts instead and you repeat this
step).

### 13. First boot of our U-Boot, save the network settings

Power-cycle the PA-220 (power cable out/in).

1. Terminal at **115200**. The first lines are garbage (Palo Alto's early
   stages print at 9600). Normal.
2. Our U-Boot: 2 s countdown, finds no kernel yet → `pa-220#`.
3. Status LEDs: STAT orange, ALM green at the prompt.

Save the TFTP settings (stored in SPI flash, kept from now on):
```
setenv ipaddr 192.168.2.2
setenv serverip 192.168.2.10
saveenv
```

## Part 3: Installing OpenWrt

### General outline of the procedure:


### 14. Install OpenWrt to the eMMC

At `pa-220#`:
```
run linux_ram
```

Press **Enter** when the boot messages stop → `root@OpenWrt:~#`. Then:
```
pa220-install 192.168.2.10
```

1. Fetches `openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar` from the PC.
2. Shows the current and the new partition table.
3. `THIS ERASES THE WHOLE eMMC. Type yes to continue:` → type `yes` →
   **Enter**.
   - **PanOS is gone after this.** Only the backup from step 11 brings it
     back.
4. Partitions, formats, installs (~1-2 min).
5. Ends with `Done. Type reboot.`

Restore settings from a LuCI backup instead of factory settings (optional,
put the file in the TFTP folder):
```
pa220-install 192.168.2.10 openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar backup-OpenWrt.tar.gz
```

Then:
```
reboot
```

### 15. First boot from the eMMC

1. Our U-Boot loads the kernel from the eMMC (2 s countdown, no key).
2. OpenWrt boots (~30 s). STAT LED turns green.
3. Front port LEDs light up when the ports come up.

eMMC layout after the install:

| Partition | Size | Contents |
|---|---|---|
| p1 | 256 MiB ext3 | Kernel `/vmlinux.oct3-mp`, mounted at `/boot` |
| p2 | Rest (~27 GiB) | ext4 root |

### 16. First login

| Where | Address |
|---|---|
| MGT port (PC gets an IP by DHCP, or keep 192.168.2.10) | 192.168.2.1 |
| Ports 1-8 (br-lan, behind a router on 192.168.1.1) | 192.168.1.2 |

1. Browser → `http://192.168.2.1` → LuCI. User `root`, no password yet.
2. System → Administration → set a root password.
3. Set up the network as you like (Network → Interfaces).

Done.

## Upgrading later

LuCI → System → Backup / Flash Firmware → flash
`openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar` (keep settings on).
Or on the console:
```
sysupgrade -v /tmp/openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar
```
Only the targz image. The previous kernel is kept as `/vmlinux.oct3-mp.bak`
(U-Boot loads it if `/vmlinux.oct3-mp` is missing).

## Recovery

- OpenWrt broken: at the U-Boot countdown press any key → `pa-220#` →
  `run linux_ram` → `pa220-install 192.168.2.10`.
- Our U-Boot broken: Palo Alto's U-Boot starts instead (9600 baud,
  `Kingfisher(ram) (mp)#`) → redo step 12.
- Our U-Boot hangs: the CPLD switches to the failsafe chip (Palo Alto's
  failsafe U-Boot, 9600). Don't write anything from there; ask for help.

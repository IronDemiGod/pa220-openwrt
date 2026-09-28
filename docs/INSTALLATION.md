# Installation Steps

## Pre-requisites

- PA-220 running PanOS (any version).
- Console cable: RJ45 console → USB serial adapter cable, or the micro-USB port.
- Terminal program (PuTTY, Tera Term, minicom, …): **9600 baud, 8N1, no
  flow control**.


## Part 1: Obtaining U-boot shell access

### General outline of the procedure:

This is a very lengthy procedure, unlike many other openwrt-compatible devices. It involves rewriting part of the bootloader, and wiping PanOS from internal storage.
Full backups of the SPI flash chips and the emmc can be taken, and therefore can be used to restore the original state of the box via U-boot and a ram-booted linux image.

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

### 6. Reboot into the U-Boot shell

1. On the result screen → **Reboot** (or main menu → **Reboot**).
2. Don't type `maint` this time.
3. U-Boot tries to load the kernel from the empty partition, fails, and
   stops at:
   ```
   Kingfisher(ram) (mp)#
   ```
4. Done. Every boot now ends at this prompt (until PanOS is reinstalled).

Check: type `printenv` → Enter. Variables are listed.

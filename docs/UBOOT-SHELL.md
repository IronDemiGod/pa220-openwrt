# Getting a U-Boot shell on a stock PA-220

Way in: empty the PanOS partition Palo Alto's U-Boot boots from. The kernel
load then fails and U-Boot drops to its prompt. Everything below is done from PanOS's own maintenance menu, with no
tools and no opening of the case.

**This destroys the active PanOS install.** The other PanOS partition, the
config and the logs are left alone. Needed once per box.

---

## What you need

- PA-220 running PanOS (any version).
- Console cable: RJ45 console → USB serial.
- Terminal program (PuTTY, Tera Term, minicom, …): **9600 baud, 8N1, no
  flow control**.

## 1. Connect the console

1. Plug the cable into the **CONSOLE** port.
2. Open the terminal at 9600 8N1.
3. Power on (or reboot) the PA-220.

## 2. Boot into maintenance mode

1. Watch the boot messages. U-Boot prints:
   ```
   Enter 'maint' to boot to maint partition.
   ```
2. Type `maint` and press **Enter** right away (the window is a few seconds).
3. It can print nothing for a while (slow kernel load). Wait.
4. The maintenance menu comes up (blue text screen, "Welcome to maintenance
   mode").

Missed the window → let it boot, then power-cycle and try again.

If you are already logged in to the PanOS CLI, this also works:
```
debug system maintenance-mode
```
→ confirm with `y`. The box reboots straight into maintenance mode.

Menu keys:

| Key | Action |
|---|---|
| ↑ ↓ / Tab | Move |
| Enter | Press button / select option |

## 3. Open "Disk Image - Advanced"

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

## 4. Find the active partition

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

## 5. Bootstrap the "maint" component into it

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

What it did: formatted the active sysroot (`mkfs.ext3`), created only
`dev/` and `var/log/`. The maint component installs nothing there → no
kernel left.

| Choice | Result |
|---|---|
| component **panos** | Reinstalls PanOS. Boots PanOS again. No shell. |
| the non-active sysroot | Wipes the backup copy. PanOS still boots. No shell. |
| **Boot** button | Only switches partition. No shell. |

## 6. Reboot into the U-Boot shell

1. On the result screen → **Reboot** (or main menu → **Reboot**).
2. Don't type `maint` this time.
3. U-Boot tries to load the kernel from the empty partition, fails, and
   stops at:
   ```
   Kingfisher(ram) (mp)#
   ```
4. Done. Every boot now ends at this prompt (until PanOS is reinstalled).

Check: type `printenv` → Enter. Variables are listed.

## Notes

- Don't boot PanOS / maint mode again after this. A PanOS boot or reinstall
  puts a kernel back → the prompt is gone and the steps above must be
  redone.
- Console stays at 9600 baud until our U-Boot is installed (it runs at
  115200).
- Log of the run on the box: `/opt/panrepo/logs/history.log` →
  `bootstrap sysroot0 maint panos-10.0.6  Success`.

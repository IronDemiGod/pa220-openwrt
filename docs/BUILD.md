# Setup for Building

### Build machine

- x86_64 Linux: Ubuntu 22.04/24.04 or Debian 12. WSL2 on Windows works too.
- ~20 GB free disk space.
- Build as a normal user, not root.
- First build takes a while (download + toolchain + kernel), later builds only
  rebuild what changed.

WSL2 only:
- Work inside the Linux file system (`~/…`), **not** `/mnt/c/…`.
- Remove the Windows paths from `PATH` before every build (OpenWrt refuses
  paths with spaces):
  ```
  export PATH=$(echo "$PATH" | tr ':' '\n' | grep -v '^/mnt/c' | paste -sd:)
  ```

### 1. Install the build dependencies

```
sudo apt update
sudo apt install build-essential file gawk git libncurses-dev python3 rsync unzip wget ca-certificates
```

### 2. Get this repo

```
cd ~
git clone https://github.com/IronDemiGod/pa220-openwrt.git
```

Layout:

| Folder | Content |
|---|---|
| `openwrt/` | PA-220 files at their OpenWrt paths + `setup.sh` + `pa220.diffconfig` |
| `openwrt/patches/` | Changes to OpenWrt's own files (image definition, sysupgrade, network defaults, kernel config, ...) as patches |
| `u-boot/` | U-Boot source (Cavium OCTEON SDK U-Boot 2013.07 + PA-220 port) + `build.sh` |
| `docs/` | Documentation |

### 3. Create the OpenWrt build tree

```
cd ~
./pa220-openwrt/openwrt/setup.sh ~/openwrt-pa220
```

What it does:
1. Clones official OpenWrt `v25.12.6` into `~/openwrt-pa220`
   (git.openwrt.org, falls back to the GitHub mirror).
2. Copies the PA-220 files from `openwrt/` into it and applies
   `openwrt/patches/*.patch` to OpenWrt's own files.
3. Updates and installs the package feeds.
4. Creates `.config` from `pa220.diffconfig` (`make defconfig`).

Only needed once.

### 4. Build OpenWrt

```
cd ~/openwrt-pa220
make -j$(nproc)
```

- Build error → run again with `make -j1 V=s` to see the full error.
- Output in `bin/targets/octeon/generic/`:

| File | Use |
|---|---|
| `openwrt-octeon-generic-pan_pa-220-initramfs-kernel.bin` | RAM image (TFTP boot, installer / recovery) |
| `openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar` | Sysupgrade |

(The `squashfs-sysupgrade.tar` is also built but is **not** usable on the PA-220.)

### 5. Making changes

Edit the files in `~/pa220-openwrt/openwrt/` (not in the build tree), then:

```
~/pa220-openwrt/openwrt/setup.sh --sync ~/openwrt-pa220
cd ~/openwrt-pa220
make -j$(nproc)
```

- `--sync` copies only changed files (unchanged files keep their time stamps,
  so `make` only rebuilds what changed).
- Files deleted in the repo are removed from the build tree / restored to
  OpenWrt's version.
- No clone, no feed update.

Changing one of OpenWrt's own files (the ones in `openwrt/patches/`, e.g.
`target/linux/octeon/image/Makefile`): edit it in the build tree, then
write the patch again from the tree's git, keeping the description lines
at the top of the patch:

```
cd ~/openwrt-pa220
git diff v25.12.6 -- target/linux/octeon/image/Makefile
```

Replace everything from the `diff --git` line on in
`openwrt/patches/0001-octeon-image-pan_pa-220.patch` with that output.
A change to a file not covered by a patch yet goes into a new patch
(`0007-...patch`, same format).

### 6. Build U-Boot (optional)

Only needed to change U-Boot. The prebuilt `u-boot-octeon_pa220.bin` is
enough otherwise.

Needs the **Cavium OCTEON SDK toolchain** (`tools-gcc-4.7`, containing
`bin/mips64-octeon-linux-gnu-gcc`). Not included in this repo, it comes
with the OCTEON SDK. The binaries are static 32-bit x86, they run on any
x86_64 Linux as they are.

Get the toolchain (once):

1. Download OCTEON SDK 5.1 (1.1 GB) from
   [jianyongchen/OCTEON-SDK](https://github.com/jianyongchen/OCTEON-SDK):
   ```
   cd ~
   wget https://github.com/jianyongchen/OCTEON-SDK/raw/master/OCTEON-SDK-5.1.tbz
   ```
2. Check the download:
   ```
   sha256sum OCTEON-SDK-5.1.tbz
   ```
   Must be `45a882624617d692acbd2bdb35d48a9a37ca7b50d5935ce24b4a58ef9c360d12`.
3. Extract only the toolchain (~4 GB tar, takes a few minutes):
   ```
   tar xjf OCTEON-SDK-5.1.tbz ./OCTEON-SDK/tools-gcc-4.7
   ```
   → `~/OCTEON-SDK/tools-gcc-4.7`. The `.tbz` can be deleted afterwards.
   The separate `tools-5.1-build-52.tgz` in that repo is not needed.

Build:

```
cd ~/pa220-openwrt/u-boot
./build.sh ~/OCTEON-SDK/tools-gcc-4.7
```

- Output: `u-boot/u-boot-octeon_pa220.bin`.
- Test a new build from RAM before flashing it (see the
  [Installation Guide](INSTALLATION.md), step 9).

### Kernel patches

`openwrt/target/linux/octeon/patches-6.12/950-…` to `962-…` change the
Linux kernel source. OpenWrt's build applies them after its own generic
and octeon patches, then compiles the kernel. Each patch starts with a
description of the change (950: the comment at the top of `pa220.c`).

| Patch | What it does |
|---|---|
| `950-pan-pa220-board-fixups` | Board code (`pa220.c`): port PHY out of reset and powered up, port MACs from the EEPROM, early STAT/TEMP LEDs, port jack LEDs |
| `951-octeon-cn70xx-agl-support` | CN70xx AGL support, ported from the OCTEON SDK |
| `952-octeon-coremask-fixup` | Uses the legacy core mask when U-Boot's extended one is garbage |
| `953-octeon-ethernet-optional-loop-netdevs` | No netdevs for the internal loopback ports (`octeon_ethernet.loop_netdevs=1` brings them back) |
| `954-octeon-populate-simple-mfd` | Creates the devices below the CPLD syscon node |
| `955-octeon-ethernet-pa220-performance` | Receive on all 4 cores, 4096 packet buffers, GRO, hardware receive checksum |
| `956-mips-octeon-aes-ghash-crypto` | AES (ECB/CBC/CTR) and GHASH on the COP2 crypto unit |
| `957-mmc-cavium-optional-no-ddr` | `cavium,no-ddr` device tree property: eMMC in SDR mode |
| `958-octeon-ignore-bootloader-mem-with-replacing-cmdline` | Ignores U-Boot's PanOS `mem=4096` when the image replaces the command line |
| `959-octeon-quiet-ubnt-pruning-fdt-crc` | UBNT pruning message only on UBNT boards, device tree checksum fixed |
| `960-octeon-ethernet-rx-tuning-options` | Run-time `rx_gro`, `rx_irq_delay`, `rx_irq_pkts` options |
| `961-octeon-ethernet-bridge-fastpath` | Bridge fast path: known unicast frames between bridge ports go straight from receive to transmit in the driver |
| `962-watchdog-octeon-report-real-timeout` | Watchdog reports its real timeout |

950–952, 954, 957 and 958 are needed for the PA-220 to work properly;
953, 959 and 962 are fixes; 955, 956, 960 and 961 are performance. The
crypto driver (956) also needs `CONFIG_CRYPTO_AES_OCTEON` from
`openwrt/patches/0005-octeon-kernel-config-pan_pa-220.patch`.

### 7. Updating to a new OpenWrt release

The PA-220 files in `openwrt/` are either new files (scripts, device tree,
kernel patches `950-…`) or patches to OpenWrt's own files
(`openwrt/patches/`). OpenWrt's changes to those files are kept; a patch
that no longer fits stops `setup.sh` instead of silently undoing them.

1. Set the new tag in `openwrt/setup.sh`: `OPENWRT_TAG=v25.12.7`.
2. Set up a **new** build tree (`--sync` does not change the OpenWrt
   version of an existing tree):
   ```
   ./pa220-openwrt/openwrt/setup.sh ~/openwrt-pa220-new
   ```
3. If it stops with `<patch> does not apply to OpenWrt v25.12.7, update
   it`: take OpenWrt's new version of that file, make the PA-220 change
   in it again and write the patch again (see step 5,
   `git diff v25.12.7 -- <file>`). Then rerun step 2. A new kernel
   version (e.g. `config-6.12` → `config-6.18`) means the kernel config
   patch needs the new file name.
4. Refresh the kernel patches `950-…` to `962-…` (see [Kernel patches](#kernel-patches)) for the new kernel:
   ```
   cd ~/openwrt-pa220-new
   make target/linux/refresh V=s
   ```
   If one of them fails to apply, fix it by hand (`make
   target/linux/{clean,prepare} V=s QUILT=1`, then `quilt push`, edit,
   `quilt refresh` in the kernel source under `build_dir/`). Copy the
   refreshed `target/linux/octeon/patches-6.12/9*.patch` back to
   `openwrt/target/linux/octeon/patches-6.12/` (only the PA-220 ones:
   the refresh also rewrites OpenWrt's own patches in that folder).
5. Build (step 4), test the new image from RAM before flashing it (see
   the [Installation Guide](INSTALLATION.md), step 9), then replace the
   old tree with the new one.

### Publishing a release

1. Write the notes: `.github/release-notes/<tag>.md` (same layout as the
   previous ones). Commit and push.
2. Build the images from that commit (step 4).
3. Create the release with the GitHub CLI, attaching the initramfs, the
   targz sysupgrade image, the U-Boot binary from the previous release
   (`gh release download <previous tag> -p u-boot-octeon_pa220.bin`)
   and a `sha256sums` of those three files:
   ```
   gh release create <tag> --target <commit> --title <tag> \
     --notes-file .github/release-notes/<tag>.md \
     openwrt-octeon-generic-pan_pa-220-initramfs-kernel.bin \
     openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar \
     u-boot-octeon_pa220.bin sha256sums
   ```

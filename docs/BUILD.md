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
  export PATH=$(echo "$PATH" | tr ':' '\n' | grep -v '^/mnt/c'
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
 SDK U-Boot 2013.07 + PA-220 port) + `build.sh` |

### 3. Create the OpenWrt build tree

```
cd ~
./pa220-openwrt/openwrt/setup.sh ~/openwrt-pa220
```

What it does:
1. Clones official OpenWrt `v25.12.4` into `~/openwrt-pa220`
   (git.openwrt.org, falls back to the GitHub mirror).

Only needed once.

### 4. Build OpenWrt

```
cd ~/openwrt-pa220
make -j$(nproc)
```

- Build error → run again with `make -j1 V=s` to see the full
- Output in `bin/targets/octeon/generic/`:

| File | Use |
|---|---|
| `openwrt-octeon-generic-pan_pa-220-initramfs-kernel.bin` | RAM image (TFTP boot, installer / recovery) |
| `openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar` | I

(The `squashfs-sysupgrade.tar` is also built but is **not** usable on the PA-220.)

### 5. Making changes

Edit the files in `~/pa220-openwrt/openwrt/` (not in the build tree), then:


- `--sync` copies only changed files (unchanged files keep their time stamps,
  so `make` only rebuilds what changed).
- Files deleted in the repo are removed from the build tree / restored to
  OpenWrt's version.
- No clone, no feed update.
- 
### 6. Build U-Boot (optional)

Only needed to change U-Boot. The prebuilt `u-boot-octeon_pa220.bin` is
enough otherwise.

Needs the **Cavium OCTEON SDK toolchain** (`tools-gcc-4.7`, containing
`bin/mips64-octeon-linux-gnu-gcc`). Not included in this repo, it comes
with the OCTEON SDK. The binaries are static 32-bit x86, they run on any
x86_64 Linux as they are.

```
cd ~/pa220-openwrt/u-boot
./build.sh /path/to/tools-gcc-4.7
```

- Output: `u-boot/u-boot-octeon_pa220.bin`.
- Test a new build from RAM before flashing it (see the
  [Installation Guide](docs/INSTALLATION.md), step 9).

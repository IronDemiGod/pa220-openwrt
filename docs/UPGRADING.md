## Upgrading OpenWrt

#### Similar to other OpenWrt devices, it is done through LuCI or the console:

LuCI → System → Backup / Flash Firmware → flash
`openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar` (keep settings on).
Or on the console:
```
sysupgrade -v /tmp/openwrt-octeon-generic-pan_pa-220-targz-sysupgrade.tar
```
Only the targz image. The previous kernel is kept as `/vmlinux.oct3-mp.bak`
(U-Boot loads it if `/vmlinux.oct3-mp` is missing).

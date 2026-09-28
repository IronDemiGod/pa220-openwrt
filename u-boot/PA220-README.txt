Palo Alto Networks PA-220 (Cavium board type 20021 "KINGFISHER") U-Boot
========================================================================

This tree is Cavium OCTEON SDK 5.1.0 U-Boot 2013.07 plus the PA-220 port
(board/octeon/pa220/, include/configs/octeon_pa220.h and a few SDK files).
The SDK "executive" files it uses are included (the SDK links them in), so
it builds without the rest of the SDK. It is the final-stage ("stage 3")
U-Boot, loaded from SPI flash by Palo Alto's own stage 1 / 1.5 / 2 (which
initialise the DRAM). It never initialises DRAM itself.

Build (Linux)
-------------
Needs the OCTEON SDK's gcc 4.7 cross compiler (mips64-octeon-linux-gnu-*,
the SDK's tools-gcc-4.7/ directory) and a host gcc.
    ./build.sh /path/to/tools-gcc-4.7
Output: u-boot-octeon_pa220.bin (Cavium bootloader header, board 20021,
image type 3).

Installing (normal SPI chip only, from U-Boot)
----------------------------------------------
Our image lives at SPI offset 0x250000 on the NORMAL chip, after Palo Alto's
own U-Boot at 0x100000 (kept as a fallback: stage 2 boots the LAST valid
stage-3 image it finds). Check "cpld read d" = 0x31 (normal chip) first.
    tftpboot 0x21000000 pa220-uboot.bin
    sf probe; sf erase 0x250000 0x14c000; sf write 0x21000000 0x250000 <size>
    sf read 0x22000000 0x250000 <size>; cmp.b 0x21000000 0x22000000 <size>
Never write the failsafe chip (CPLD reg d bit 0 = 0 / bit 2 = 1 selects it).
Saved environment (saveenv): 0xe00000 and 0xe10000 on the normal chip.

Network
-------
No IP addresses are compiled in. Before the first TFTP transfer:
    setenv ipaddr <address for the PA-220>; setenv serverip <TFTP server>
    saveenv
(or "dhcp" for ipaddr). TFTP uses the MGT port (octrgmii0).
"run linux_ram" loads ${ramimage} (default: the OpenWrt initramfs image
openwrt-octeon-generic-pan_pa-220-initramfs-kernel.bin) from ${serverip}
and boots it from RAM.

Device tree
-----------
pa220.dts describes the board for U-Boot only (OpenWrt has its own). MAC
addresses are placeholders; U-Boot fills them in from the board EEPROM
(MAC base, 9 addresses) at boot.

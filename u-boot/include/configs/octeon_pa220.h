/*
 * Palo Alto Networks PA-220 (CN7130, Cavium board type 20021 "KINGFISHER").
 *
 * SDK U-Boot intended as the SPI stage 3 (loaded by Palo Alto's stage 2
 * into initialised DRAM) or chain-loaded from RAM for testing. It never
 * initialises DRAM itself. Environment lives in RAM only: nothing is ever
 * written to flash by this build.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __CONFIG_H__
#define __CONFIG_H__

#define CONFIG_OCTEON_CN70XX

#ifndef CONFIG_OCTEON_PCI_HOST
# define CONFIG_OCTEON_PCI_HOST		0
#endif

#define CONFIG_OCTEON_USB_OCTEON3	/** Enable USB support on OCTEON III */

/* No NOR flash on the boot bus (only the CPLD); boot device is SPI NOR */
#define CONFIG_SYS_NO_FLASH

#include "octeon_common.h"

#undef CONFIG_OCTEON_I2C_LOW_LEVEL

#define CONFIG_LBA48			/* 48-bit mode */
#define CONFIG_SYS_64BIT_LBA		/* 64-bit LBA support */
#define CONFIG_SYS_ATA_BASE_ADDR	0 /* Make compile happy */

/* eMMC: 8-bit, 26 MHz (52 MHz gave no gain; reads are overhead-bound) */
#define CONFIG_OCTEON_MMC		/* Enable MMC support */
#define CONFIG_MMC_MBLOCK		/* Multi-block support */
#define CONFIG_CMD_MMC			/* Enable mmc command */
#define CONFIG_SYS_MMC_SET_DEV		/* Enable multiple MMC devices */
#define CONFIG_MMC
#define CONFIG_OCTEON_MMC_MAX_FREQUENCY	26000000
/* Never write the one-time-programmable EXT_CSD RST_n_FUNCTION field */
#define CONFIG_OCTEON_MMC_NO_RST_N	1

/*
 * Palo Alto's SPI stage 2 has already cleared (and ECC-initialised) all DRAM
 * before starting us, so don't clear the 8 GB a second time.
 */
#define CONFIG_NO_CLEAR_DDR		1

/* Board TLV EEPROM (board type 20021, serial, MAC base) */
#define CONFIG_SYS_I2C_EEPROM_ADDR		0x57
#define CONFIG_SYS_DEF_EEPROM_ADDR		CONFIG_SYS_I2C_EEPROM_ADDR
#define CONFIG_SYS_EEPROM_PAGE_WRITE_BITS	6	/* 24c256: 64 bytes */
#define CONFIG_SYS_EEPROM_PAGE_WRITE_DELAY_MS	5
/*
 * The board EEPROM only answers 1-byte (8-bit) offsets: with the SDK default
 * of 2 every read starts at offset 0, so the TLV parser never finds the MAC
 * or board descriptor (random MAC, serial# "unknown").
 */
#undef CONFIG_SYS_I2C_EEPROM_ADDR_LEN
#define CONFIG_SYS_I2C_EEPROM_ADDR_LEN		1

/*
 * Autoboot OpenWrt: kernel /vmlinux.oct3-mp on eMMC p1, root on p2 (the
 * kernel command line sets it). Falls back to the previous kernel (.bak,
 * kept by sysupgrade).
 */
#define CONFIG_BOOTDELAY	2	/* autoboot after X seconds */
#define CONFIG_BOOTCOMMAND	\
	"ext2load mmc 0:1 0x21000000 /vmlinux.oct3-mp || " \
	"ext2load mmc 0:1 0x21000000 /vmlinux.oct3-mp.bak; " \
	"bootoctlinux 0x21000000 numcores=4 endbootargs"
#undef	CONFIG_BOOTARGS

/* Networking: front ports are QSGMII (88E1680), MGT is AGL (octrgmii0) */
#define CONFIG_OCTEON_SGMII_ENET
#define CONFIG_OCTEON_QSGMII_ENET
#define CONFIG_OCTEON_MGMT_ENET

#if defined(CONFIG_OCTEON_RGMII_ENET) || defined(CONFIG_OCTEON_SGMII_ENET) || \
	defined(CONFIG_OCTEON_XAUI_ENET) || defined(CONFIG_OCTEON_QSGMII_ENET)
# define CONFIG_OCTEON_INTERNAL_ENET
#endif

#define CONFIG_OCTEON_BOOTCMD

/* Configure QLM (DLM0 = QSGMII/QSGMII, see board_configure_qlms) */
#define CONFIG_OCTEON_QLM

#include "octeon_cmd_conf.h"

#ifdef  CONFIG_CMD_NET
# define CONFIG_PHY_GIGE
# define CONFIG_PHY_MARVELL
# include "octeon_network.h"
#endif

#define CONFIG_CMD_OCTEON_TLVEEPROM
#define CONFIG_CMD_EXT2			/** EXT2/3 filesystem support	*/
#define CONFIG_CMD_EXT4			/** EXT4 filesystem support	*/
#define CONFIG_CMD_FAT			/** FAT support			*/

/* SPI NOR (Winbond W25Q128) — read access only is intended */
#define CONFIG_SF_DEFAULT_BUS		0
#define CONFIG_SF_DEFAULT_CS		0
#define CONFIG_SF_DEFAULT_SPEED		16000000
#define CONFIG_OCTEON_SPI		/** Enable OCTEON SPI driver	*/
#define CONFIG_SPI			/** Enable SPI support		*/
#define CONFIG_SPI_FLASH		/** Enable SPI flash driver	*/
#define CONFIG_SPI_FLASH_WINBOND	/** Winbond W25Q128		*/
#define CONFIG_CMD_SPI			/** Enable SPI command		*/
#define CONFIG_CMD_SF			/** Enable SPI flash command	*/

/*
 * dram_size_mbytes: DRAM is always set up by Palo Alto's stage 2, which
 * passes its own dram_size_mbytes (read first, so it wins). This default only
 * matters when we are started without it (e.g. "go" from another U-Boot for
 * RAM testing); without any value U-Boot would try to set up the DRAM itself
 * and stop, as it has no DDR settings for this board.
 *
 * No network addresses are preset. For TFTP (e.g. "run linux_ram", which
 * RAM-boots the OpenWrt initramfs image named in ramimage, from the MGT port)
 * set them once:  setenv ipaddr <this box>; setenv serverip <TFTP server>;
 * saveenv  (or get ipaddr from a DHCP server with "dhcp").
 */
#define	CONFIG_EXTRA_ENV_SETTINGS					\
	"dram_size_mbytes=8192\0"					\
	"autoload=n\0"							\
	"ethact=octrgmii0\0"						\
	"ethprime=octrgmii0\0"						\
	"loadaddr=0x21000000\0"						\
	"ramimage=openwrt-octeon-generic-pan_pa-220-initramfs-kernel.bin\0" \
	"linux_ram=tftpboot ${loadaddr} ${serverip}:${ramimage} && "	\
		"bootoctlinux ${loadaddr} numcores=4 endbootargs mem=0\0"

/*
 * Environment (saveenv) on the NORMAL SPI chip, in space that is unused on
 * this board (0x250000-0xffffff was erased; our U-Boot uses 0x250000-
 * 0x39afff; Palo Alto's own env partition at 0xef0000 is left alone).
 * Two 64 KiB copies so a power cut during saveenv can't lose it.
 */
#define CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SPI_BUS		0
#define CONFIG_ENV_SPI_CS		0
#define CONFIG_ENV_SPI_MAX_HZ		16000000
#define CONFIG_ENV_SPI_MODE		0
#define CONFIG_ENV_SIZE			(8*1024)
#define CONFIG_ENV_SECT_SIZE		(64*1024)
#define CONFIG_ENV_OFFSET		0xe00000
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_OFFSET_REDUND	0xe10000

/* ---- Extra commands ("List 1") ---- */

/* Scripting: if/then, test, true/false, exit (hush shell) */
#define CONFIG_SYS_HUSH_PARSER
#define CONFIG_SYS_PROMPT_HUSH_PS2	"> "
/* hush uses this fixed string instead of the Octeon auto prompt ("=> ") */
#define CONFIG_SYS_PROMPT		"pa-220# "
/* Pressing Enter on an empty line does nothing (no command repeat) */
#define CONFIG_SYS_NO_CMD_REPEAT

/* Environment */
#define CONFIG_CMD_EXPORTENV		/* env export */
#define CONFIG_CMD_IMPORTENV		/* env import */
#define CONFIG_CMD_BOOTD		/* boot, bootd */
#define CONFIG_MENU
#define CONFIG_CMD_BOOTMENU		/* bootmenu */
#define CONFIG_CMD_INI			/* ini */
#define CONFIG_CMD_TERMINAL		/* terminal */
#define CONFIG_CMD_GETTIME		/* gettime */
#define CONFIG_CMD_TIMER		/* timer */

/* Storage and files */
#define CONFIG_CMD_EXT4_WRITE		/* ext4write */
#define CONFIG_FAT_WRITE		/* fatwrite */
#define CONFIG_CMD_FS_GENERIC		/* load, ls */
#define CONFIG_PARTITION_UUIDS
#define CONFIG_EFI_PARTITION
#define CONFIG_CMD_PART			/* part */
#define CONFIG_CMD_GPT			/* gpt */
#define CONFIG_CMD_READ			/* read */

/* Networking */
#define CONFIG_CMD_SNTP			/* sntp */
#define CONFIG_CMD_LINK_LOCAL		/* linklocal */
#define CONFIG_LIB_RAND
#define CONFIG_CMD_PXE			/* pxe, sysboot */
#define CONFIG_SYS_ARCH			"mips"	/* PXE default file names */

/* Hardware and diagnostics */
#define CONFIG_CMD_DATE			/* date: DS1338 RTC on I2C bus 0 */
#define CONFIG_RTC_DS1338
#define CONFIG_SYS_I2C_RTC_ADDR		0x68
#define CONFIG_CMD_MEMTEST		/* mtest */
#define CONFIG_SYS_MEMTEST_START	0x22000000
#define CONFIG_SYS_MEMTEST_END		0x2a000000
#define CONFIG_CMD_OCTEON_BOOTBUS	/* octbootbus */
#define CONFIG_HW_WATCHDOG		/* octwd; only runs if watchdog_enable is set */
#define CONFIG_MX_CYCLIC		/* mdc, mwc (+64) */
#define CONFIG_LOOPW			/* loopw (+64) */
#define CONFIG_CMD_IMI			/* iminfo */
#define CONFIG_SHA256
#define CONFIG_CMD_HASH			/* hash (sha256, ...) */
#define CONFIG_HASH_VERIFY
#define CONFIG_GZIP_COMPRESSED
#define CONFIG_CMD_ZIP			/* zip */
/* cpld: custom command in board/octeon/pa220/pa220_board.c */

#endif	/* __CONFIG_H__ */

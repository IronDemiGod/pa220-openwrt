/*
 * Palo Alto Networks PA-220 (Cavium board type 20021, "KINGFISHER").
 *
 * Minimal board support derived from the EVB7000 board file. Intended to
 * run as the SPI stage 3 U-Boot (loaded into already-initialised DRAM by
 * Palo Alto's stage 2), or chain-loaded from RAM for testing.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <command.h>
#include <asm/mipsregs.h>
#include <asm/arch/octeon_boot.h>
#include <asm/arch/octeon_board_common.h>
#include <asm/arch/octeon_fdt.h>
#include <asm/arch/lib_octeon_shared.h>
#include <asm/arch/lib_octeon.h>
#include <asm/gpio.h>
#include <asm/arch/octeon_boot_bus.h>
#include <environment.h>
#include <libfdt.h>
#include <asm/arch/cvmx-qlm.h>
#include <asm/arch/octeon_qlm.h>

DECLARE_GLOBAL_DATA_PTR;

/* Only used for DRAM initialisation, which this U-Boot never does itself */
#define PA220_DEF_DRAM_FREQ	667

/*
 * The CPLD (status LEDs, PHY reset, ...) sits on boot bus chip select 0 at
 * 0x1b040000. start.S disables CS0 on boards without NOR flash, so map it
 * again from the /soc/bootbus node (Palo Alto's DT: range + cs-config@0
 * timings), like Palo Alto's U-Boot does. Linux relies on this window.
 */
int octeon_add_user_boot_bus_fdt_devices(void)
{
	static const char *cpld_compat[] = { "kingfisher-mp,cpld", NULL };

	return octeon_boot_bus_add_fdt_handler("pan-cpld", (void *)cpld_compat,
					       &octeon_boot_bus_generic_init);
}

/* Report the CPLD mapping; reg 0 (version) is read-only, so reading is safe */
int octeon_boot_bus_board_post_init(const void *fdt_addr)
{
	cvmx_mio_boot_reg_cfgx_t reg_cfg;

	reg_cfg.u64 = cvmx_read_csr(CVMX_MIO_BOOT_REG_CFGX(0));
	if (!reg_cfg.s.en || reg_cfg.s.base != (0x1b040000 >> 16)) {
		printf("CPLD:  boot bus CS0 not mapped (cfg 0x%llx)\n",
		       (unsigned long long)reg_cfg.u64);
		return 0;
	}
	/*
	 * Tell the CPLD this (normal flash) bootloader is alive: pulse reg d
	 * bit 0 low then high, exactly as Palo Alto's U-Boot does in its board
	 * init. Without it the CPLD boot watchdog switches the board over to
	 * the failsafe SPI flash a few seconds later.
	 */
	{
		uint8_t d = cvmx_read64_uint8(CVMX_ADD_IO_SEG(0x1b04000dull)) & 0xfe;

		cvmx_write64_uint8(CVMX_ADD_IO_SEG(0x1b04000dull), d);
		udelay(10000);
		cvmx_write64_uint8(CVMX_ADD_IO_SEG(0x1b04000dull), d | 1);
		udelay(10000);
	}
	printf("CPLD:  version %u, reg 5 = 0x%02x, reg d = 0x%02x (cfg 0x%llx tim 0x%llx)\n",
	       cvmx_read64_uint8(CVMX_ADD_IO_SEG(0x1b040000ull)),
	       cvmx_read64_uint8(CVMX_ADD_IO_SEG(0x1b040005ull)),
	       cvmx_read64_uint8(CVMX_ADD_IO_SEG(0x1b04000dull)),
	       (unsigned long long)reg_cfg.u64,
	       (unsigned long long)cvmx_read_csr(CVMX_MIO_BOOT_REG_TIMX(0)));
	return 0;
}

int checkboard(void)
{
	printf("Board: Palo Alto Networks PA-220 (SDK U-Boot)\n");
	return 0;
}

/*
 * Map the CPLD (boot bus CS0) as early as possible and switch the front
 * panel STAT LED to orange: CPLD reg 7 bit 5 = STAT orange, bit 4 = STAT
 * green. CPLD registers survive a reset, so HA (bits 0-1) and ALM green
 * (bit 2, "in the U-Boot shell", see board_enter_cli()) are switched off;
 * ALM red (bit 3, kernel panic) is left alone. TEMP (reg 6 bits 0-1) is
 * switched off until Linux sets it.
 * start.S disabled CS0, so set it up here from the same /soc/bootbus DT data
 * used again later by octeon_boot_bus_late_init().
 */
static void pa220_early_stat_led(void)
{
	static const char *cpld_compat[] = { "kingfisher-mp,cpld", NULL };
	uint64_t reg6 = CVMX_ADD_IO_SEG(0x1b040006ull);
	uint64_t reg7 = CVMX_ADD_IO_SEG(0x1b040007ull);
	int bootbus, node;

	bootbus = fdt_path_offset(gd->fdt_blob, "/soc/bootbus");
	if (bootbus < 0)
		return;
	node = fdt_subnode_offset(gd->fdt_blob, bootbus, "pan-cpld@0,0");
	if (node < 0)
		return;
	if (octeon_boot_bus_generic_init(gd->fdt_blob, node, bootbus,
					 (void *)cpld_compat, NULL))
		return;
	cvmx_write64_uint8(reg7, (cvmx_read64_uint8(reg7) & ~0x37) | 0x20);
	/*
	 * TEMP LED (reg 6 bits 0-1) off: it keeps OpenWrt's state across a
	 * reboot; Linux sets it again when it boots. Other reg 6 bits have
	 * unknown functions, so read-modify-write.
	 */
	cvmx_write64_uint8(reg6, cvmx_read64_uint8(reg6) & ~0x03);
}

int early_board_init(void)
{
	int cpu_ref = DEFAULT_CPU_REF_FREQUENCY_MHZ;

	pa220_early_stat_led();

	/* Keep USB power off until Linux enables it (GPIO 2/3) */
	gpio_direction_output(2, 0);
	gpio_direction_output(3, 0);

	/* Board type, serial and MAC addresses from the TLV EEPROM */
	octeon_board_get_clock_info(PA220_DEF_DRAM_FREQ);
	octeon_board_get_descriptor(CVMX_BOARD_TYPE_PA220, 1, 0);

	gd->arch.ddr_ref_hertz = DEFAULT_CPU_REF_FREQUENCY_MHZ * 1000 * 1000ull;

	octeon_board_get_mac_addr();

	gd->cpu_clk = octeon_get_cpu_multiplier() * cpu_ref * 1000000;

	return 0;
}

/*
 * With CONFIG_ENV_IS_NOWHERE the SDK never imports the default environment
 * (env_relocate() is only called for flash/EEPROM/RAM environments), so
 * nothing but the variables passed by Palo Alto's stage 2 would exist and
 * GD_FLG_ENV_READY would stay clear, making setenv()/getenv() from C
 * silently fail (no autoboot, TFTP "serverip not set"). Load the defaults
 * here; the passed-in variables are merged on top right after this.
 */
int early_board_init_r(void)
{
	if (!(gd->flags & GD_FLG_ENV_READY))
		set_default_env(NULL);
	return 0;
}

/*
 * DLM0 carries the two QSGMII links to the 88E1680 (front ports 1-4 and
 * 5-8); DLM1/2 are unused. Same call as Palo Alto's U-Boot: 2500 MBd,
 * reference clock select 0, reference clock input 1. Linux relies on the
 * bootloader for this.
 */
void board_configure_qlms(void)
{
	puts("Configuring DLM0 for QSGMII/QSGMII\n");
	octeon_configure_qlm(0, 2500, CVMX_QLM_MODE_QSGMII_QSGMII, 0, 0, 0, 1);
}

/*
 * U-Boot's 88E1680 driver (m88e1680s_config) powers every front-port PHY up
 * when the network is initialised, so the ports would link and blink under
 * Linux even with their interfaces down. Hand the chip over the way Palo
 * Alto's U-Boot does: held in reset (CPLD reg 5 bit 2). Linux releases the
 * reset and the PHYs start powered down until a port is brought up.
 */
void board_prepare_os_handoff(void)
{
	cvmx_mio_boot_reg_cfgx_t reg_cfg;
	uint64_t reg5 = CVMX_ADD_IO_SEG(0x1b040005ull);
	uint64_t reg7 = CVMX_ADD_IO_SEG(0x1b040007ull);

	reg_cfg.u64 = cvmx_read_csr(CVMX_MIO_BOOT_REG_CFGX(0));
	if (!reg_cfg.s.en || reg_cfg.s.base != (0x1b040000 >> 16))
		return;
	cvmx_write64_uint8(reg5, cvmx_read64_uint8(reg5) | 0x04);

	/* Leaving U-Boot: ALM green ("in the U-Boot shell") off */
	cvmx_write64_uint8(reg7, cvmx_read64_uint8(reg7) & ~0x04);
}

/*
 * Autoboot was interrupted (or failed) and the command prompt is about to
 * start: light ALM green (CPLD reg 7 bit 2) while we sit in the U-Boot shell.
 */
void board_enter_cli(void)
{
	cvmx_mio_boot_reg_cfgx_t reg_cfg;
	uint64_t reg7 = CVMX_ADD_IO_SEG(0x1b040007ull);

	reg_cfg.u64 = cvmx_read_csr(CVMX_MIO_BOOT_REG_CFGX(0));
	if (!reg_cfg.s.en || reg_cfg.s.base != (0x1b040000 >> 16))
		return;
	cvmx_write64_uint8(reg7, cvmx_read64_uint8(reg7) | 0x04);
}

/*
 * cpld: read/write the PA-220 CPLD registers (boot bus CS0, 0x1b040000).
 *   0 version, 1 power good, 5 bit2 = 88E1680 reset, 6 TEMP LED,
 *   7 HA/ALM/STAT LEDs, d = flash select / reboot (needs -f to write).
 */
static int pa220_cpld_mapped(void)
{
	cvmx_mio_boot_reg_cfgx_t reg_cfg;

	reg_cfg.u64 = cvmx_read_csr(CVMX_MIO_BOOT_REG_CFGX(0));
	return reg_cfg.s.en && reg_cfg.s.base == (0x1b040000 >> 16);
}

static int do_cpld(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned long reg, val;
	int force = 0;

	if (!pa220_cpld_mapped()) {
		puts("CPLD not mapped (boot bus CS0 disabled)\n");
		return CMD_RET_FAILURE;
	}
	if (argc < 2 || !strcmp(argv[1], "display")) {
		for (reg = 0; reg <= 0xd; reg++)
			printf("Reg %lx = 0x%02x\n", reg,
			       cvmx_read64_uint8(CVMX_ADD_IO_SEG(0x1b040000ull + reg)));
		return CMD_RET_SUCCESS;
	}
	if (!strcmp(argv[1], "read") && argc == 3) {
		reg = simple_strtoul(argv[2], NULL, 16);
		if (reg > 0xd)
			return CMD_RET_USAGE;
		printf("Reg %lx = 0x%02x\n", reg,
		       cvmx_read64_uint8(CVMX_ADD_IO_SEG(0x1b040000ull + reg)));
		return CMD_RET_SUCCESS;
	}
	if (!strcmp(argv[1], "write") && argc >= 4) {
		if (argc == 5 && !strcmp(argv[2], "-f")) {
			force = 1;
			argv++;
		} else if (argc != 4) {
			return CMD_RET_USAGE;
		}
		reg = simple_strtoul(argv[2], NULL, 16);
		val = simple_strtoul(argv[3], NULL, 16);
		if (reg > 0xd || val > 0xff)
			return CMD_RET_USAGE;
		if (reg == 0xd && !force) {
			puts("Reg d selects the flash chip and can reboot the "
			     "board; use 'cpld write -f d <val>' if you mean it\n");
			return CMD_RET_FAILURE;
		}
		cvmx_write64_uint8(CVMX_ADD_IO_SEG(0x1b040000ull + reg), val);
		printf("Reg %lx = 0x%02x\n", reg,
		       cvmx_read64_uint8(CVMX_ADD_IO_SEG(0x1b040000ull + reg)));
		return CMD_RET_SUCCESS;
	}
	return CMD_RET_USAGE;
}

U_BOOT_CMD(cpld, 5, 0, do_cpld,
	   "read/write PA-220 CPLD registers",
	   "[display]            - show registers 0-d\n"
	   "cpld read <reg>           - read one register (hex)\n"
	   "cpld write <reg> <val>    - write one register (hex)\n"
	   "cpld write -f d <val>     - write reg d (flash select/reboot!)");

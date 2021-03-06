/*
 * DENX M53 module
 *
 * Copyright (C) 2012-2013 Marek Vasut <marex@denx.de>
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/crm_regs.h>
#include <asm/arch/clock.h>
#include <asm/arch/iomux-mx53.h>
#include <asm/arch/spl.h>
#include <asm/errno.h>
#include <netdev.h>
#include <i2c.h>
#include <mmc.h>
#include <spl.h>
#include <fsl_esdhc.h>
#include <asm/gpio.h>
#include <usb/ehci-fsl.h>

DECLARE_GLOBAL_DATA_PTR;

int dram_init(void)
{
	u32 size1, size2;

	size1 = get_ram_size((void *)PHYS_SDRAM_1, PHYS_SDRAM_1_SIZE);
	size2 = get_ram_size((void *)PHYS_SDRAM_2, PHYS_SDRAM_2_SIZE);

	gd->ram_size = size1 + size2;

	return 0;
}
void dram_init_banksize(void)
{
	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = PHYS_SDRAM_1_SIZE;

	gd->bd->bi_dram[1].start = PHYS_SDRAM_2;
	gd->bd->bi_dram[1].size = PHYS_SDRAM_2_SIZE;
}

static void setup_iomux_uart(void)
{
	static const iomux_v3_cfg_t uart_pads[] = {
		MX53_PAD_PATA_BUFFER_EN__UART2_RXD_MUX,
		MX53_PAD_PATA_DMARQ__UART2_TXD_MUX,
	};

	imx_iomux_v3_setup_multiple_pads(uart_pads, ARRAY_SIZE(uart_pads));
}

#ifdef CONFIG_USB_EHCI_MX5
int board_ehci_hcd_init(int port)
{
	if (port == 0) {
		/* USB OTG PWRON */
		imx_iomux_v3_setup_pad(NEW_PAD_CTRL(MX53_PAD_GPIO_4__GPIO1_4,
					PAD_CTL_PKE | PAD_CTL_DSE_HIGH));
		gpio_direction_output(IMX_GPIO_NR(1, 4), 0);

		/* USB OTG Over Current */
		imx_iomux_v3_setup_pad(MX53_PAD_GPIO_18__GPIO7_13);
	} else if (port == 1) {
		/* USB Host PWRON */
		imx_iomux_v3_setup_pad(NEW_PAD_CTRL(MX53_PAD_GPIO_2__GPIO1_2,
					PAD_CTL_PKE | PAD_CTL_DSE_HIGH));
		gpio_direction_output(IMX_GPIO_NR(1, 2), 0);

		/* USB Host Over Current */
		imx_iomux_v3_setup_pad(MX53_PAD_GPIO_3__USBOH3_USBH1_OC);
	}

	return 0;
}
#endif

static void setup_iomux_fec(void)
{
	static const iomux_v3_cfg_t fec_pads[] = {
		/* MDIO pads */
		NEW_PAD_CTRL(MX53_PAD_FEC_MDIO__FEC_MDIO, PAD_CTL_HYS |
			PAD_CTL_DSE_HIGH | PAD_CTL_PUS_22K_UP | PAD_CTL_ODE),
		NEW_PAD_CTRL(MX53_PAD_FEC_MDC__FEC_MDC, PAD_CTL_DSE_HIGH),

		/* FEC 0 pads */
		NEW_PAD_CTRL(MX53_PAD_FEC_CRS_DV__FEC_RX_DV,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_FEC_REF_CLK__FEC_TX_CLK,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_FEC_RX_ER__FEC_RX_ER,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_FEC_TX_EN__FEC_TX_EN, PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_FEC_RXD0__FEC_RDATA_0,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_FEC_RXD1__FEC_RDATA_1,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_FEC_TXD0__FEC_TDATA_0, PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_FEC_TXD1__FEC_TDATA_1, PAD_CTL_DSE_HIGH),

		/* FEC 1 pads */
		NEW_PAD_CTRL(MX53_PAD_KEY_COL0__FEC_RDATA_3,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_KEY_ROW0__FEC_TX_ER,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_KEY_COL1__FEC_RX_CLK,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_KEY_ROW1__FEC_COL,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_KEY_COL2__FEC_RDATA_2,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_KEY_ROW2__FEC_TDATA_2, PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_KEY_COL3__FEC_CRS,
				PAD_CTL_HYS | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_GPIO_19__FEC_TDATA_3, PAD_CTL_DSE_HIGH),
	};

	imx_iomux_v3_setup_multiple_pads(fec_pads, ARRAY_SIZE(fec_pads));
}

#ifdef CONFIG_FSL_ESDHC
struct fsl_esdhc_cfg esdhc_cfg = {
	MMC_SDHC1_BASE_ADDR,
};

int board_mmc_getcd(struct mmc *mmc)
{
	imx_iomux_v3_setup_pad(MX53_PAD_GPIO_1__GPIO1_1);
	gpio_direction_input(IMX_GPIO_NR(1, 1));

	return !gpio_get_value(IMX_GPIO_NR(1, 1));
}

#define SD_CMD_PAD_CTRL		(PAD_CTL_HYS | PAD_CTL_DSE_HIGH | \
				 PAD_CTL_PUS_100K_UP)
#define SD_PAD_CTRL		(PAD_CTL_HYS | PAD_CTL_PUS_47K_UP | \
				 PAD_CTL_DSE_HIGH)

int board_mmc_init(bd_t *bis)
{
	static const iomux_v3_cfg_t sd1_pads[] = {
		NEW_PAD_CTRL(MX53_PAD_SD1_CMD__ESDHC1_CMD, SD_CMD_PAD_CTRL),
		NEW_PAD_CTRL(MX53_PAD_SD1_CLK__ESDHC1_CLK, SD_PAD_CTRL),
		NEW_PAD_CTRL(MX53_PAD_SD1_DATA0__ESDHC1_DAT0, SD_PAD_CTRL),
		NEW_PAD_CTRL(MX53_PAD_SD1_DATA1__ESDHC1_DAT1, SD_PAD_CTRL),
		NEW_PAD_CTRL(MX53_PAD_SD1_DATA2__ESDHC1_DAT2, SD_PAD_CTRL),
		NEW_PAD_CTRL(MX53_PAD_SD1_DATA3__ESDHC1_DAT3, SD_PAD_CTRL),
		MX53_PAD_EIM_DA13__GPIO3_13,

		MX53_PAD_EIM_EB3__GPIO2_31, /* SD power */
	};

	esdhc_cfg.sdhc_clk = mxc_get_clock(MXC_ESDHC_CLK);

	imx_iomux_v3_setup_multiple_pads(sd1_pads, ARRAY_SIZE(sd1_pads));

	/* GPIO 2_31 is SD power */
	gpio_direction_output(IMX_GPIO_NR(2, 31), 0);

	return fsl_esdhc_initialize(bis, &esdhc_cfg);
}
#endif

#define I2C_PAD_CTRL	(PAD_CTL_SRE_FAST | PAD_CTL_DSE_HIGH | \
			 PAD_CTL_PUS_100K_UP | PAD_CTL_ODE)

static void setup_iomux_i2c(void)
{
	static const iomux_v3_cfg_t i2c_pads[] = {
		NEW_PAD_CTRL(MX53_PAD_EIM_D16__I2C2_SDA, I2C_PAD_CTRL),
		NEW_PAD_CTRL(MX53_PAD_EIM_EB2__I2C2_SCL, I2C_PAD_CTRL),
	};

	imx_iomux_v3_setup_multiple_pads(i2c_pads, ARRAY_SIZE(i2c_pads));
}

static void setup_iomux_nand(void)
{
	static const iomux_v3_cfg_t nand_pads[] = {
		NEW_PAD_CTRL(MX53_PAD_NANDF_WE_B__EMI_NANDF_WE_B,
				PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_NANDF_RE_B__EMI_NANDF_RE_B,
				PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_NANDF_CLE__EMI_NANDF_CLE,
				PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_NANDF_ALE__EMI_NANDF_ALE,
				PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_NANDF_WP_B__EMI_NANDF_WP_B,
				PAD_CTL_PUS_100K_UP),
		NEW_PAD_CTRL(MX53_PAD_NANDF_RB0__EMI_NANDF_RB_0,
				PAD_CTL_PUS_100K_UP),
		NEW_PAD_CTRL(MX53_PAD_NANDF_CS0__EMI_NANDF_CS_0,
				PAD_CTL_DSE_HIGH),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA0__EMI_NANDF_D_0,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA1__EMI_NANDF_D_1,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA2__EMI_NANDF_D_2,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA3__EMI_NANDF_D_3,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA4__EMI_NANDF_D_4,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA5__EMI_NANDF_D_5,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA6__EMI_NANDF_D_6,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
		NEW_PAD_CTRL(MX53_PAD_PATA_DATA7__EMI_NANDF_D_7,
				PAD_CTL_DSE_HIGH | PAD_CTL_PKE),
	};

	imx_iomux_v3_setup_multiple_pads(nand_pads, ARRAY_SIZE(nand_pads));
}

static void m53_set_clock(void)
{
	int ret;
	const uint32_t ref_clk = MXC_HCLK;
	const uint32_t dramclk = 400;
	uint32_t cpuclk;

	imx_iomux_v3_setup_pad(NEW_PAD_CTRL(MX53_PAD_GPIO_10__GPIO4_0,
					    PAD_CTL_DSE_HIGH | PAD_CTL_PKE));
	gpio_direction_input(IMX_GPIO_NR(4, 0));

	/* GPIO10 selects modules' CPU speed, 1 = 1200MHz ; 0 = 800MHz */
	cpuclk = gpio_get_value(IMX_GPIO_NR(4, 0)) ? 1200 : 800;

	ret = mxc_set_clock(ref_clk, cpuclk, MXC_ARM_CLK);
	if (ret)
		printf("CPU:   Switch CPU clock to %dMHz failed\n", cpuclk);

	ret = mxc_set_clock(ref_clk, dramclk, MXC_PERIPH_CLK);
	if (ret) {
		printf("CPU:   Switch peripheral clock to %dMHz failed\n",
			dramclk);
	}

	ret = mxc_set_clock(ref_clk, dramclk, MXC_DDR_CLK);
	if (ret)
		printf("CPU:   Switch DDR clock to %dMHz failed\n", dramclk);
}

static void m53_set_nand(void)
{
	u32 i;

	/* NAND flash is muxed on ATA pins */
	setbits_le32(M4IF_BASE_ADDR + 0xc, M4IF_GENP_WEIM_MM_MASK);

	/* Wait for Grant/Ack sequence (see EIM_CSnGCR2:MUX16_BYP_GRANT) */
	for (i = 0x4; i < 0x94; i += 0x18) {
		clrbits_le32(WEIM_BASE_ADDR + i,
			     WEIM_GCR2_MUX16_BYP_GRANT_MASK);
	}

	mxc_set_clock(0, 33, MXC_NFC_CLK);
	enable_nfc_clk(1);
}

int board_early_init_f(void)
{
	setup_iomux_uart();
	setup_iomux_fec();
	setup_iomux_i2c();
	setup_iomux_nand();

	m53_set_clock();

	mxc_set_sata_internal_clock();

	/* NAND clock @ 33MHz */
	m53_set_nand();

	return 0;
}

int board_init(void)
{
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

int checkboard(void)
{
	puts("Board: DENX M53EVK\n");

	return 0;
}

/*
 * NAND SPL
 */
#ifdef CONFIG_SPL_BUILD
void spl_board_init(void)
{
	setup_iomux_nand();
	m53_set_clock();
	m53_set_nand();
}

u32 spl_boot_device(void)
{
	return BOOT_DEVICE_NAND;
}
#endif

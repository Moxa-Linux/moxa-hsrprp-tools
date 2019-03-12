/*
/*
 * Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 *
 * Moxa HSR/PRP Card library
 *
 * 2017-09-18 Holsety Chen
 *   new release
 *
 */

#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include "mxhsrprp.h"

/*
 * Avalon Slave Select
 */
#define AVALON_SLAVE_SELECT_SWITCH	0x1
#define AVALON_SLAVE_SELECT_PORT_I	0x2
#define AVALON_SLAVE_SELECT_PORT_A	0x4
#define AVALON_SLAVE_SELECT_PORT_B	0x8

/*
 * FRS Port Configuration Registers
 */

/* General Configuration and State Registers */
#define PORT_CFG_REG_GROUP_GEN	(0x0000)
#define PORT_STATE		(PORT_CFG_REG_GROUP_GEN + 0x0000)
#define PORT_VLAN		(PORT_CFG_REG_GROUP_GEN + 0x0008)
#define PORT_VLAN0_MAPPING	(PORT_CFG_REG_GROUP_GEN + 0x0009)
#define PORT_FWD_MASK		(PORT_CFG_REG_GROUP_GEN + 0x000A)
#define PORT_VLAN_PRIO		(PORT_CFG_REG_GROUP_GEN + 0x000B)

/* HSR configuration registers */
#define PORT_CFG_REG_GROUP_HSR	(0x1000)
#define HSR_CFG			(PORT_CFG_REG_GROUP_HSR + 0x0000)

/* PTP configuration registers */
#define PORT_CFG_REG_GROUP_PTP	(0x2000)
#define PTP_DELAY_SUBNS		(PORT_CFG_REG_GROUP_PTP + 0x0000)
#define PTP_DELAY_NS_LOW	(PORT_CFG_REG_GROUP_PTP + 0x0001)
#define PTP_DELAY_NS_HIGH	(PORT_CFG_REG_GROUP_PTP + 0x0002)

/* Counter Registers */
#define PORT_CFG_REG_GROUP_CNT	(0x3000)
#define CNT_CTRL 		(PORT_CFG_REG_GROUP_CNT + 0x0000)
#define RX_GOOD_OCTETS_L	(PORT_CFG_REG_GROUP_CNT + 0x0100)
#define RX_GOOD_OCTETS_H	(PORT_CFG_REG_GROUP_CNT + 0x0101)
#define RX_BAD_OCTETS_L		(PORT_CFG_REG_GROUP_CNT + 0x0102)
#define RX_BAD_OCTETS_H		(PORT_CFG_REG_GROUP_CNT + 0x0103)
#define RX_UNICAST_L		(PORT_CFG_REG_GROUP_CNT + 0x0104)
#define RX_UNICAST_H		(PORT_CFG_REG_GROUP_CNT + 0x0105)
#define RX_BROADCAST_L		(PORT_CFG_REG_GROUP_CNT + 0x0106)
#define RX_BROADCAST_H		(PORT_CFG_REG_GROUP_CNT + 0x0107)
#define RX_MULTICAST_L		(PORT_CFG_REG_GROUP_CNT + 0x0108)
#define RX_MULTICAST_H		(PORT_CFG_REG_GROUP_CNT + 0x0109)
#define RX_UNDERSIZE_L		(PORT_CFG_REG_GROUP_CNT + 0x010A)
#define RX_UNDERSIZE_H		(PORT_CFG_REG_GROUP_CNT + 0x010B)
#define RX_FRAGMENTS_L		(PORT_CFG_REG_GROUP_CNT + 0x010C)
#define RX_FRAGMENTS_H		(PORT_CFG_REG_GROUP_CNT + 0x010D)
#define RX_OVERSIZE_L		(PORT_CFG_REG_GROUP_CNT + 0x010E)
#define RX_OVERSIZE_H		(PORT_CFG_REG_GROUP_CNT + 0x010F)
#define RX_JABBER_L		(PORT_CFG_REG_GROUP_CNT + 0x0110)
#define RX_JABBER_H		(PORT_CFG_REG_GROUP_CNT + 0x0111)
#define RX_ERR_L		(PORT_CFG_REG_GROUP_CNT + 0x0112)
#define RX_ERR_H		(PORT_CFG_REG_GROUP_CNT + 0x0113)
#define RX_CRC_L		(PORT_CFG_REG_GROUP_CNT + 0x0114)
#define RX_CRC_H		(PORT_CFG_REG_GROUP_CNT + 0x0115)
#define RX_64_L			(PORT_CFG_REG_GROUP_CNT + 0x0116)
#define RX_64_H			(PORT_CFG_REG_GROUP_CNT + 0x0117)
#define RX_65_127_L		(PORT_CFG_REG_GROUP_CNT + 0x0118)
#define RX_65_127_H		(PORT_CFG_REG_GROUP_CNT + 0x0119)
#define RX_128_255_L		(PORT_CFG_REG_GROUP_CNT + 0x011A)
#define RX_128_255_H		(PORT_CFG_REG_GROUP_CNT + 0x011B)
#define RX_256_511_L		(PORT_CFG_REG_GROUP_CNT + 0x011C)
#define RX_256_511_H		(PORT_CFG_REG_GROUP_CNT + 0x011D)
#define RX_512_1023_L		(PORT_CFG_REG_GROUP_CNT + 0x011E)
#define RX_512_1023_H		(PORT_CFG_REG_GROUP_CNT + 0x011F)
#define RX_1024_1536_L		(PORT_CFG_REG_GROUP_CNT + 0x0120)
#define RX_1024_1536_H		(PORT_CFG_REG_GROUP_CNT + 0x0121)
#define RX_HSRPRP_L		(PORT_CFG_REG_GROUP_CNT + 0x0122)
#define RX_HSRPRP_H		(PORT_CFG_REG_GROUP_CNT + 0x0123)
#define RX_WRONGLAN_L		(PORT_CFG_REG_GROUP_CNT + 0x0124)
#define RX_WRONGLAN_H		(PORT_CFG_REG_GROUP_CNT + 0x0125)
#define RX_DUPLICATE_L		(PORT_CFG_REG_GROUP_CNT + 0x0126)
#define RX_DUPLICATE_H		(PORT_CFG_REG_GROUP_CNT + 0x0127)
#define TX_OCTETS_L		(PORT_CFG_REG_GROUP_CNT + 0x0140)
#define TX_OCTETS_H		(PORT_CFG_REG_GROUP_CNT + 0x0141)
#define TX_UNICAST_L		(PORT_CFG_REG_GROUP_CNT + 0x0142)
#define TX_UNICAST_H		(PORT_CFG_REG_GROUP_CNT + 0x0143)
#define TX_BROADCAST_L		(PORT_CFG_REG_GROUP_CNT + 0x0144)
#define TX_BROADCAST_H		(PORT_CFG_REG_GROUP_CNT + 0x0145)
#define TX_MULTICAST_L		(PORT_CFG_REG_GROUP_CNT + 0x0146)
#define TX_MULTICAST_H		(PORT_CFG_REG_GROUP_CNT + 0x0147)
#define TX_HSRPRP_L		(PORT_CFG_REG_GROUP_CNT + 0x0148)
#define TX_HSRPRP_H		(PORT_CFG_REG_GROUP_CNT + 0x0149)
#define PRIQ_DROP_L		(PORT_CFG_REG_GROUP_CNT + 0x0160)
#define PRIQ_DROP_H		(PORT_CFG_REG_GROUP_CNT + 0x0161)
#define EARLY_DROP_L		(PORT_CFG_REG_GROUP_CNT + 0x0162)
#define EARLY_DROP_H		(PORT_CFG_REG_GROUP_CNT + 0x0163)

/* Inbound policy registers */
#define PORT_CFG_REG_GROUP_IPO	(0x4000)
#define ETH_ADDR_CFG(n)		(PORT_CFG_REG_GROUP_IPO + 0x0000 + n*0x10)
#define ETH_ADDR0_FWD_ALLOW(n)	(PORT_CFG_REG_GROUP_IPO + 0x0001 + n*0x10)
#define ETH_ADDR0_FWD_MIRROR(n)	(PORT_CFG_REG_GROUP_IPO + 0x0002 + n*0x10)
#define ETH_ADDR0_0(n)		(PORT_CFG_REG_GROUP_IPO + 0x0004 + n*0x10)
#define ETH_ADDR0_1(n)		(PORT_CFG_REG_GROUP_IPO + 0x0005 + n*0x10)
#define ETH_ADDR0_2(n)		(PORT_CFG_REG_GROUP_IPO + 0x0006 + n*0x10)

/*
 * Structure of Generic Configuration and State Registers
 */
typedef union
{
	__u16 v;
	struct {
		__u16 forwarding_state	: 2;
		__u16 management_state	: 2;
		__u16 hw_mode		: 2;
		__u16 reserved		: 2;
		__u16 speed_select	: 2;
		__u16 current_speed	: 2;
		__u16 reserved2		: 4;
	} bit;
} port_state_t;

#define FORWARDING_STATE_FORWARDING		0
#define FORWARDING_STATE_LEARNING		1
#define FORWARDING_STATE_DISABLED		2
#define MANAGEMENT_STATE_NORMAL			0
#define MANAGEMENT_STATE_MANAGEMENT_MODE	1
#define HW_MODE_MII				0
#define HW_MODE_GMII				2
#define SPEED_SELECT_EXTERNAL			0
#define SPEED_SELECT_1000M			1
#define SPEED_SELECT_100M			2
#define SPEED_SELECT_10M			3
#define CURRENT_SPEED_1000M			1
#define CURRENT_SPEED_100M			2
#define CURRENT_SPEED_10M			3

/*
 * Structure of HSR/PRP Configuration Register
 */
typedef union
{
	__u16 v;
	struct {
		__u16 port_mode		: 1;
		__u16 reserved		: 7;
		__u16 hsr_prp_mode	: 1;
		__u16 red_intlnk_mode	: 1;
		__u16 lan_id		: 1;
		__u16 net_id		: 3;
		__u16 reserved2		: 2;
	} bit;
} hsr_cfg_t;

#define PORT_MODE_DISABLE			0
#define PORT_MODE_ENABLE			1
#define HSR_PRP_MODE_HSR			0
#define HSR_PRP_MODE_PRP			1
#define RED_INTLNK_MODE_REDUNDANT		0
#define RED_INTLNK_MODE_INTERLINK		1


/*
 * SMBUS to MDIO / Avalon BUS Controller
 */
#define SMBUS_SLAVE_ADDRESS(idx)	(0x58 + (idx&0x7))

/* SMBUS command code */
#define CMD_GET_MDIO_BUS_STATUS			0x1F
#define CMD_SET_MDIO_PHY_REG_ADDR		0x00
#define CMD_GET_MDIO_PHY_REG_ADDR		0x01
#define CMD_WRITE_MDIO_PHY_REG_ADDR_DATA	0x02
#define CMD_WRITE_MDIO_PHY_REG_DATA		0x03
#define CMD_READ_MDIO_PHY_REG_DATA		0x04
#define CMD_WRITE_AVALON_REG(addr)		(0x30 + (addr&0xf))
#define CMD_READ_AVALON_REG(addr)		(0x50 + (addr&0xf))
#define CMD_SET_GPIO_MODE_7_0			0xC0
#define CMD_SET_GPIO_MODE_15_8			0xC1
#define CMD_GET_GPIO_MODE_7_0			0xC2
#define CMD_GET_GPIO_MODE_15_8			0xC3
#define CMD_SET_GPIO_PIN_7_0			0xC4
#define CMD_SET_GPIO_PIN_15_8			0xC5
#define CMD_GET_GPIO_PIN_7_0			0xC6
#define CMD_GET_GPIO_PIN_15_8			0xC7
#define CMD_GET_FPGA_VERSION			0xE0

/*
 * PHY....
 */
#define PHY_ADDR_LAN_A		0x4
#define PHY_ADDR_LAN_B		0x6
#define PHY_ADDR_INTERLINK	0x7


#define PHY_REG_PHY_EXTENDED_CONTROL	0x10

/* PHY REG 0x1C access */
#define PHY_REG_1C_WE			(1<<15)
#define PHY_REG_1C(shadow_reg, data)	(((shadow_reg&0x1f)<<10) | (data&0x3ff))

#define SPARE_CONTROL_1			0x02
#define AUTO_DETECT_MEDIUM		0x1E
#define MODE_CONTROL			0x1F

/*
 * FPGA GPIO for LED
 */
#define LED_HSR		(1<<0)
#define LED_PRP		(1<<1)
#define LED_FAULT	(1<<2)

int write_avalon_reg( int fd, __u8 slave_select, __u16 address, __u16 data)
{
	__s32 res;
	__u8 block_data[4];

	address |= 0x8000; /* avalon bus write request */
	block_data[0] = (address >> 8) & 0xff;
	block_data[1] = address & 0xff;
	block_data[2] = (data >> 8) & 0xff;
	block_data[3] = data & 0xff;

	res = i2c_smbus_write_block_data(fd, CMD_WRITE_AVALON_REG(slave_select),
		sizeof(block_data), block_data);
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_write_block_data error: %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

int read_avalon_reg(int fd, __u8 slave_select, __u16 address, __u16 *data)
{
	__s32 res;

#if 0
	res = i2c_smbus_process_call(fd, CMD_READ_AVALON_REG(slave_select),
		(((address<<8)&0xff00) | ((address>>8)&0xff)));
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_process_call error: %s\n",
			strerror(errno));
		return -1;
	}
#else
	res = i2c_smbus_write_word_data(fd, CMD_READ_AVALON_REG(slave_select),
		(((address<<8)&0xff00) | ((address>>8)&0xff)));
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_write_word_data error: %s\n",
			strerror(errno));
		return -1;
	}

	res = i2c_smbus_read_word_data(fd, CMD_READ_AVALON_REG(slave_select));
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_read_word_data error: %s\n",
			strerror(errno));
		return -1;
	}
#endif

	*data = ((res << 8) & 0xff00) | ((res >> 8) & 0xff);
	return 0;
}

int write_phy_reg(int fd, __u8 phy_addr, __u8 phy_reg, __u16 data)
{
	__s32 res;
	union {
		__u16 val_16;
		__u8 val_8[2];
	} v;

	v.val_8[0] = phy_addr;
	v.val_8[1] = phy_reg;
	res = i2c_smbus_write_word_data(fd, CMD_SET_MDIO_PHY_REG_ADDR, v.val_16);
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_write_word_data error: %s\n",
			strerror(errno));
		return -1;
	}

	res = i2c_smbus_read_word_data(fd, CMD_GET_MDIO_PHY_REG_ADDR);
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_read_word_data error: %s\n",
			strerror(errno));
		return -1;
	}

	res = i2c_smbus_write_word_data(fd, CMD_WRITE_MDIO_PHY_REG_DATA, 
		(((data<<8)&0xff00) | ((data>>8)&0xff)));
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_write_word_data error: %s\n",
			strerror(errno));
		return -1;
	}

	return 0;
}

int read_phy_reg(int fd, __u8 phy_addr, __u8 phy_reg, __u16 *data)
{
	__s32 res;
	union {
		__u16 val_16;
		__u8 val_8[2];
	} v;

	v.val_8[0] = phy_addr;
	v.val_8[1] = phy_reg;
	res = i2c_smbus_write_word_data(fd, CMD_SET_MDIO_PHY_REG_ADDR, v.val_16);
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_write_word_data error: %s\n",
			strerror(errno));
		return -1;
	}

	res = i2c_smbus_read_word_data(fd, CMD_GET_MDIO_PHY_REG_ADDR);
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_read_word_data error: %s\n",
			strerror(errno));
		return -1;
	}

	res = i2c_smbus_read_word_data(fd, CMD_READ_MDIO_PHY_REG_DATA);
	if ( res < 0 ) {
		fprintf(stderr, "i2c_smbus_read_word_data error: %s\n",
			strerror(errno));
		return -1;
	}
	*data = ((res << 8) & 0xff00) | ((res >> 8) & 0xff);

	return 0;
}

int config_port(int fd, __u8 slave_select, int mode)
{
	port_state_t port_state;
	hsr_cfg_t hsr_cfg;
	__u16 port_fwd_mask;

	/* Prepare HSR/PRP Configuration Register */
	hsr_cfg.v = 0;
	if (slave_select == AVALON_SLAVE_SELECT_PORT_I) {
		hsr_cfg.bit.port_mode = PORT_MODE_DISABLE;
	} else {
		hsr_cfg.bit.port_mode = PORT_MODE_ENABLE;
		if (slave_select == AVALON_SLAVE_SELECT_PORT_A) {
			hsr_cfg.bit.lan_id = 0;
			port_fwd_mask = 0x4; /* mask port B */
		} else if (slave_select == AVALON_SLAVE_SELECT_PORT_B) {
			hsr_cfg.bit.lan_id = 1;
			port_fwd_mask = 0x2; /* mask port A */
		}

		if ( mode == MODE_PRP ) {
			hsr_cfg.bit.hsr_prp_mode = HSR_PRP_MODE_PRP;
		} else if ( mode == MODE_HSR ) {
			hsr_cfg.bit.hsr_prp_mode = HSR_PRP_MODE_HSR;
			port_fwd_mask = 0x0; /* No mask needed in HSR mode */
		}
	}

	/* Disable port forwarding */
	if (read_avalon_reg(fd, slave_select, PORT_STATE, &port_state.v) < 0)
		return -1;
	port_state.bit.forwarding_state = FORWARDING_STATE_DISABLED;
	if (write_avalon_reg(fd, slave_select, PORT_STATE, port_state.v) < 0)
		return -1;

	/* HSR/PRP Configuration Register */
	if (write_avalon_reg(fd, slave_select, HSR_CFG, hsr_cfg.v) < 0)
		return -1;

	/* Configure port foward mask */
	if (slave_select != AVALON_SLAVE_SELECT_PORT_I) {
		if (write_avalon_reg(fd, slave_select, PORT_FWD_MASK,
			port_fwd_mask) < 0)
			return -1;
	}

	/* Enable port forwarding */
	port_state.bit.forwarding_state = FORWARDING_STATE_FORWARDING;
	if (write_avalon_reg(fd, slave_select, PORT_STATE, port_state.v) < 0)
		return -1;

	return 0;
}

int get_fpga_version(int fd, __u8 *major, __u8 *minor)
{
	__s32 res;
	res = i2c_smbus_read_word_data(fd, CMD_GET_FPGA_VERSION);
	if ( res < 0 ) {
		return -1;
	}

	*major = (res >> 8) & 0xff;
	*minor = res & 0xff;
	return 0;
}

int set_gpio_mode_l(int fd, __u8 clr_bit, __u8 set_bit)
{
	__s32 res;
	__u8 data;
	res = i2c_smbus_read_byte_data(fd, CMD_GET_GPIO_MODE_7_0);
	if ( res < 0 ) {
		return -1;
	}

	data = (res & ~clr_bit) | set_bit;
	if (i2c_smbus_write_byte_data(fd, CMD_SET_GPIO_MODE_7_0, data) < 0)
		return -1;

	return 0;
}

int set_gpio_pin_l(int fd, __u8 clr_bit, __u8 set_bit)
{
	__s32 res;
	__u8 data;
	res = i2c_smbus_read_byte_data(fd, CMD_GET_GPIO_PIN_7_0);
	if ( res < 0 ) {
		return -1;
	}

	data = (res & ~clr_bit) | set_bit;
	if (i2c_smbus_write_byte_data(fd, CMD_SET_GPIO_PIN_7_0, data) < 0)
		return -1;

	return 0;
}

int get_gpio_pin_l(int fd, __u8 *data)
{
	__s32 res;

	res = i2c_smbus_read_byte_data(fd, CMD_GET_GPIO_PIN_7_0);
	if ( res < 0 ) {
		return -1;
	}

	*data = res;

	return 0;
}

int init_card(int fd, int index)
{
	__u16 v;
	__u8 major;
	__u8 minor;

	if (ioctl(fd, I2C_SLAVE, SMBUS_SLAVE_ADDRESS(index)) != 0) {
		fprintf(stderr, "ioctl error: %s\n", strerror(errno));
		return -1;
	}
	/* try to get fpga version */
	if (get_fpga_version(fd, &major, &minor) < 0)
		return -1;
	printf("Card %d FPGA version is %x.%x\n", index, major, minor);

	/* Reset all counter */
	if (write_avalon_reg(fd, AVALON_SLAVE_SELECT_PORT_I, CNT_CTRL, 0x1) < 0)
		return -1;
	if (write_avalon_reg(fd, AVALON_SLAVE_SELECT_PORT_A, CNT_CTRL, 0x1) < 0)
		return -1;
	if (write_avalon_reg(fd, AVALON_SLAVE_SELECT_PORT_B, CNT_CTRL, 0x1) < 0)
		return -1;

	/* Configure LEDs */
	if (set_gpio_mode_l(fd, LED_HSR | LED_PRP | LED_FAULT, 0x0) < 0)
		return -1;
	#if 0
	if (set_gpio_pin_l(fd, 0x0, LED_HSR | LED_PRP | LED_FAULT) < 0)
		return -1;
	#else
		falut_led_enable(fd, 0);
	#endif

	/* Enable Link Speed LED mode for LAN A */
	v = PHY_REG_1C_WE | PHY_REG_1C(SPARE_CONTROL_1, (1<<2));
	if (write_phy_reg(fd, PHY_ADDR_LAN_A, 0x1C, v) < 0)
		return -1;

	/* Enable Link Speed LED mode for LAN B */
	v = PHY_REG_1C_WE | PHY_REG_1C(SPARE_CONTROL_1, (1<<2));
	if (write_phy_reg(fd, PHY_ADDR_LAN_B, 0x1C, v) < 0)
		return -1;

	/* Work around for Fiber LED: bit 8 should be 1*/
	v = PHY_REG_1C(AUTO_DETECT_MEDIUM, 0);
	if (write_phy_reg(fd, PHY_ADDR_LAN_A, 0x1C, v) < 0)
		return -1;
	if (read_phy_reg(fd, PHY_ADDR_LAN_A, 0x1C, &v) < 0)
		return -1;
	v = PHY_REG_1C_WE | PHY_REG_1C(AUTO_DETECT_MEDIUM, v | (1<<8));
	if (write_phy_reg(fd, PHY_ADDR_LAN_A, 0x1C, v) < 0)
		return -1;

	v = PHY_REG_1C(AUTO_DETECT_MEDIUM, 0);
	if (write_phy_reg(fd, PHY_ADDR_LAN_B, 0x1C, v) < 0)
		return -1;
	if (read_phy_reg(fd, PHY_ADDR_LAN_B, 0x1C, &v) < 0)
		return -1;
	v = PHY_REG_1C_WE | PHY_REG_1C(AUTO_DETECT_MEDIUM, v | (1<<8));
	if (write_phy_reg(fd, PHY_ADDR_LAN_B, 0x1C, v) < 0)
		return -1;

	#if 0 /* Dump phy registers for debug*/
	{
	unsigned char reg;
	printf("LAN A PHY, 0x1c regs\n");
	for (reg=0;reg<0x20;reg++) {
		v = PHY_REG_1C(reg, 0);
		if (write_phy_reg(fd, PHY_ADDR_LAN_A, 0x1C, v) < 0)
			return -1;
		if (read_phy_reg(fd, PHY_ADDR_LAN_A, 0x1C, &v) < 0)
			return -1;
		printf("reg %x: %x\n", reg, v&0x3ff);
	}
	printf("LAN B PHY, 0x1c regs\n");
	for (reg=0;reg<0x20;reg++) {
		v = PHY_REG_1C(reg, 0);
		if (write_phy_reg(fd, PHY_ADDR_LAN_B, 0x1C, v) < 0)
			return -1;
		if (read_phy_reg(fd, PHY_ADDR_LAN_B, 0x1C, &v) < 0)
			return -1;
		printf("reg %x: %x\n", reg, v&0x3ff);
	}
	}
	#endif

	return 0;
}

int get_prp_counters(int fd, int port, struct prp_counters *counters)
{
	__u8 slave_select;
	union {
		__u32 val_32;
		__u16 val_16[2];
	} v;

	if (port == PORT_I) {
		slave_select = AVALON_SLAVE_SELECT_PORT_I;
	} else if (port == PORT_A) {
		slave_select = AVALON_SLAVE_SELECT_PORT_A;
	} else if (port == PORT_B) {
		slave_select = AVALON_SLAVE_SELECT_PORT_B;
	} else {
		return -1;
	}

	/* capture counters */
	if (write_avalon_reg(fd, slave_select, CNT_CTRL, 0x1) < 0)
		return -1;

	/* Reset counter only */
	if ( counters == NULL )
		return 0;

	/* rx_good_octets */
	if (read_avalon_reg(fd, slave_select, RX_GOOD_OCTETS_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_GOOD_OCTETS_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_good_octets += v.val_32;

	/* rx_bad_octets */
	if (read_avalon_reg(fd, slave_select, RX_BAD_OCTETS_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_BAD_OCTETS_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_bad_octets += v.val_32;

	/* rx_unicast */
	if (read_avalon_reg(fd, slave_select, RX_UNICAST_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_UNICAST_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_unicast += v.val_32;

	/* rx_broadcast */
	if (read_avalon_reg(fd, slave_select, RX_BROADCAST_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_BROADCAST_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_broadcast += v.val_32;

	/* rx_multicast */
	if (read_avalon_reg(fd, slave_select, RX_MULTICAST_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_MULTICAST_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_multicast += v.val_32;

	/* rx_undersize */
	if (read_avalon_reg(fd, slave_select, RX_UNDERSIZE_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_UNDERSIZE_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_undersize += v.val_32;

	/* rx_fragments */
	if (read_avalon_reg(fd, slave_select, RX_FRAGMENTS_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_FRAGMENTS_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_fragments += v.val_32;

	/* rx_oversize */
	if (read_avalon_reg(fd, slave_select, RX_OVERSIZE_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_OVERSIZE_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_oversize += v.val_32;

	/* rx_jabber */
	if (read_avalon_reg(fd, slave_select, RX_JABBER_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_JABBER_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_jabber += v.val_32;

	/* rx_err */
	if (read_avalon_reg(fd, slave_select, RX_ERR_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_ERR_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_err += v.val_32;

	/* rx_crc */
	if (read_avalon_reg(fd, slave_select, RX_CRC_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_CRC_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_crc += v.val_32;

	/* rx_64 */
	if (read_avalon_reg(fd, slave_select, RX_64_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_64_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_64 += v.val_32;

	/* rx_65_127 */
	if (read_avalon_reg(fd, slave_select, RX_65_127_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_65_127_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_65_127 += v.val_32;

	/* rx_128_255 */
	if (read_avalon_reg(fd, slave_select, RX_128_255_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_128_255_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_128_255 += v.val_32;

	/* rx_256_511 */
	if (read_avalon_reg(fd, slave_select, RX_256_511_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_256_511_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_256_511 += v.val_32;

	/* rx_512_1023 */
	if (read_avalon_reg(fd, slave_select, RX_512_1023_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_512_1023_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_512_1023 += v.val_32;

	/* rx_1024_1536 */
	if (read_avalon_reg(fd, slave_select, RX_1024_1536_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_1024_1536_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_1024_1536 += v.val_32;

	/* rx_hsrprp */
	if (read_avalon_reg(fd, slave_select, RX_HSRPRP_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_HSRPRP_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_hsrprp += v.val_32;

	/* rx_wronglan */
	if (read_avalon_reg(fd, slave_select, RX_WRONGLAN_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_WRONGLAN_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_wronglan += v.val_32;

	/* rx_duplicate */
	if (read_avalon_reg(fd, slave_select, RX_DUPLICATE_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, RX_DUPLICATE_H, &v.val_16[1]) < 0)
		return -1;
	counters->rx_duplicate += v.val_32;

	/* tx_octets */
	if (read_avalon_reg(fd, slave_select, TX_OCTETS_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, TX_OCTETS_H, &v.val_16[1]) < 0)
		return -1;
	counters->tx_octets += v.val_32;

	/* tx_unicast */
	if (read_avalon_reg(fd, slave_select, TX_UNICAST_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, TX_UNICAST_H, &v.val_16[1]) < 0)
		return -1;
	counters->tx_unicast += v.val_32;

	/* tx_broadcast */
	if (read_avalon_reg(fd, slave_select, TX_BROADCAST_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, TX_BROADCAST_H, &v.val_16[1]) < 0)
		return -1;
	counters->tx_broadcast += v.val_32;

	/* tx_multicast */
	if (read_avalon_reg(fd, slave_select, TX_MULTICAST_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, TX_MULTICAST_H, &v.val_16[1]) < 0)
		return -1;
	counters->tx_multicast += v.val_32;

	/* tx_hsrprp */
	if (read_avalon_reg(fd, slave_select, TX_HSRPRP_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, TX_HSRPRP_H, &v.val_16[1]) < 0)
		return -1;
	counters->tx_hsrprp += v.val_32;

	/* priq_drop */
	if (read_avalon_reg(fd, slave_select, PRIQ_DROP_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, PRIQ_DROP_H, &v.val_16[1]) < 0)
		return -1;
	counters->priq_drop += v.val_32;

	/* early_drop */
	if (read_avalon_reg(fd, slave_select, EARLY_DROP_L, &v.val_16[0]) < 0)
		return -1;
	if (read_avalon_reg(fd, slave_select, EARLY_DROP_H, &v.val_16[1]) < 0)
		return -1;
	counters->early_drop += v.val_32;

	return 0;
}

int set_prp_mode(int fd, int mode)
{
	__u16 v;
	__u8 clr_bit = 0;
	__u8 set_bit = 0;

	if (mode == MODE_PRP) {
		/* clear hsr led */
		#ifdef HSRLED_HA
		clr_bit |= LED_HSR;
		#else
		set_bit |= LED_HSR;
		#endif
		/* set prp led */
		#ifdef PRPLED_HA
		set_bit |= LED_PRP;
		#else
		clr_bit |= LED_PRP;
		#endif
	} else if (mode == MODE_HSR) {
		/* clear prp led */
		#ifdef PRPLED_HA
		clr_bit |= LED_PRP;
		#else
		set_bit |= LED_PRP;
		#endif
		/* set hsr led */
		#ifdef HSRLED_HA
		set_bit |= LED_HSR;
		#else
		clr_bit |= LED_HSR;
		#endif
	} else {
		return -1;
	}

	/* Configure interlink/lanA/lanB port */
	if (config_port(fd, AVALON_SLAVE_SELECT_PORT_I, mode) < 0)
		return -1;
	if (config_port(fd, AVALON_SLAVE_SELECT_PORT_A, mode) < 0)
		return -1;
	if (config_port(fd, AVALON_SLAVE_SELECT_PORT_B, mode) < 0)
		return -1;

	/* Turn on LED */
	if (set_gpio_pin_l(fd, clr_bit, set_bit) < 0)
		return -1;


	return 0;
}

int falut_led_enable(int fd, int enable)
{
	__u8 pin_set;
	__u8 pin_clr;

	#ifdef FALUTLED_HA
	if (enable)
		enable = 0;
	else
		enable = 1;
	#endif

	if (enable) {
		pin_clr = LED_FAULT;
		pin_set = 0x0;
	} else {
		pin_clr = 0x0;
		pin_set = LED_FAULT;
	}
		
	if (set_gpio_pin_l(fd, pin_clr, pin_set) < 0)
		return -1;

	return 0;
}

int get_link_status(int fd, int port, int *status)
{
	__u8 phy_addr;
	__u16 v;

	if (port == PORT_I) {
		phy_addr = PHY_ADDR_INTERLINK;
	} else if (port == PORT_A) {
		phy_addr = PHY_ADDR_LAN_A;
	} else if (port == PORT_B) {
		phy_addr = PHY_ADDR_LAN_B;
	} else {
		return -1;
	}

	v = PHY_REG_1C(MODE_CONTROL, 0);
	if (write_phy_reg(fd, phy_addr, 0x1C, v) < 0)
		return -1;
	if (read_phy_reg(fd, phy_addr, 0x1C, &v) < 0)
		return -1;

	if ( v & (0x3<<6) )
		*status = 1;
	else
		*status = 0;

	return 0;
}

int get_link_speed(int fd, int port, int *speed)
{
	__u8 slave_select;
	port_state_t port_state;

	if (port == PORT_I) {
		slave_select = AVALON_SLAVE_SELECT_PORT_I;
	} else if (port == PORT_A) {
		slave_select = AVALON_SLAVE_SELECT_PORT_A;
	} else if (port == PORT_B) {
		slave_select = AVALON_SLAVE_SELECT_PORT_B;
	} else {
		return -1;
	}

	if (read_avalon_reg(fd, slave_select, PORT_STATE, &port_state.v) < 0)
		return -1;

	if (port_state.bit.speed_select == SPEED_SELECT_1000M) {
		*speed = 1000;
	} else if (port_state.bit.speed_select == SPEED_SELECT_100M) {
		*speed = 100;
	} else if (port_state.bit.speed_select == SPEED_SELECT_10M) {
		*speed = 10;
	} else if (port_state.bit.speed_select == SPEED_SELECT_EXTERNAL) {
		if (port_state.bit.current_speed == 0x1) {
			*speed = 1000;
		} else if (port_state.bit.current_speed == 0x2) {
			*speed = 100;
		} else if (port_state.bit.current_speed == 0x3) {
			*speed = 10;
		} else {
			*speed = 0;
		}
	} else {
		return -1;
	}

	return 0;
}

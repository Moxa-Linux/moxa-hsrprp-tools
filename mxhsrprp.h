/*
 * Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 *
 * Moxa HSR/PRP Card library header
 *
 * 2017-09-18 Holsety Chen
 *   new release
 */

#ifndef _MXHSRPRP_H
#define _MXHSRPRP_H

#define MAX_PRPHSR_CARD	8

/*
 * HSR/PRP Mode
 */
#define MODE_PRP	0
#define MODE_HSR	1

/*
 * Port index
 */
#define PORT_I		0 /* Inter link port */
#define PORT_A		1 /* LAN A port */
#define PORT_B		2 /* LAN B port */

/**
 * struct prp_counters - prp counters
 *
 * @rx_good_octets:	rx
 */
struct prp_counters {
	__u64 rx_good_octets;
	__u64 rx_bad_octets;
	__u64 rx_unicast;
	__u64 rx_broadcast;
	__u64 rx_multicast;
	__u64 rx_undersize;
	__u64 rx_fragments;
	__u64 rx_oversize;
	__u64 rx_jabber;
	__u64 rx_err;
	__u64 rx_crc;
	__u64 rx_64;
	__u64 rx_65_127;
	__u64 rx_128_255;
	__u64 rx_256_511;
	__u64 rx_512_1023;
	__u64 rx_1024_1536;
	__u64 rx_hsrprp;
	__u64 rx_wronglan;
	__u64 rx_duplicate;
	__u64 tx_octets;
	__u64 tx_unicast;
	__u64 tx_broadcast;
	__u64 tx_multicast;
	__u64 tx_hsrprp;
	__u64 priq_drop;
	__u64 early_drop;
};

int init_card(int fd, int index);
int get_prp_counters(int fd, int port, struct prp_counters *counters);
int set_prp_mode(int fd, int mode);
int falut_led_enable(int fd, int enable);
int get_link_status(int fd, int port, int *status);
int get_link_speed(int fd, int port, int *speed);

#endif // _MXHSRPRP_H

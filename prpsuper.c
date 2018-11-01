/*
 * This progarm use to send supervision frame to Moxa HSR/PRP Card.
 * The FPGA will regenerate current supervision frame to LAN A/B.
 *
 * 2017-09-18 Holsety Chen
 *   new release
 *
 */

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>

#define DEST_MAC0	0x01
#define DEST_MAC1	0x15
#define DEST_MAC2	0x4E
#define DEST_MAC3	0x00
#define DEST_MAC4	0x01
#define DEST_MAC5	0x00

#define BUF_SIZE			128
#define DEFAULT_IF			"eth0"
#define DEFAULT_UPDATE_PERIOD_SEC	2
#define DEFAULT_PRP_MODE		0

extern int optind, opterr, optopt;
extern char *optarg;

void usage(void)
{
	printf("This progarm use to send supervision frame to Moxa HSR/PRP Card.\n\n");
	printf("Usage:\n");
	printf("\t-h: Show this information.\n");
	printf("\t-t: send HSR/PRP supervision frame period.\
\n\t    Default is %d second.\n", DEFAULT_UPDATE_PERIOD_SEC);
	printf("\t-m: send HSR or PRP supervision frame mode.\
\n\t    0 is PRP; 1 is HSR. Default is %d.\n", DEFAULT_PRP_MODE);
	printf("\t-i: interface name, Ex.: eth0.\n");
	printf("\t    Execute chk-mx-prp-card to list supported interface.\n");
	printf("\t-a: Destination MAC address last byte.\
\n\t    The range is 00~FF. Default is 00\n");
	printf("\n\n");
}

int main(int argc, char *argv[])
{
	char c;
	char optstring[] = "hi:t:m:a:";
	int sockfd;
	struct ifreq if_idx;
	struct ifreq if_mac;
	int tx_len = 0;
	char sendbuf[BUF_SIZE];
	struct ether_header *eh = (struct ether_header *) sendbuf;
	struct iphdr *iph = (struct iphdr *) (sendbuf + sizeof(struct ether_header));
	struct sockaddr_ll socket_address;
	char ifName[IFNAMSIZ] = {0};
	unsigned char macaddr_lastbyte = DEST_MAC5;
	int polling_sec = DEFAULT_UPDATE_PERIOD_SEC;
	int mode = DEFAULT_PRP_MODE;
	int seqnum = 0;

	if (argc == 1) {
		usage();
		return 0;
	}

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			return 0;
		case 'i':
			strcpy(ifName, optarg);
			break;
		case 't':
			polling_sec = atoi(optarg);
			if (polling_sec < 0) {
				fprintf(stderr, "Bad argument!\n");
				exit(-1);
			}
			break;
		case 'm':
			mode = atoi(optarg);
			if (mode < 0) {
				fprintf(stderr, "Bad argument!\n");
				exit(-1);
			} else if (mode > 1) {
				fprintf(stderr, "Not support!\n");
				exit(-1);
			}
			break;
		case 'a':
			macaddr_lastbyte = strtol(optarg, NULL, 16);
			break;
		}
	}

	if (strlen(ifName) == 0) {
		fprintf(stderr, "Should input interface name\n");
		exit(-1);
	}

	/* Open RAW socket to send on */
	if ((sockfd = socket(AF_PACKET, SOCK_RAW, IPPROTO_RAW)) == -1) {
		perror("socket");
	}

	/* Get the index of the interface to send on */
	memset(&if_idx, 0, sizeof(struct ifreq));
	strncpy(if_idx.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFINDEX, &if_idx) < 0) {
		perror("SIOCGIFINDEX");
	}

	/* Get the MAC address of the interface to send on */
	memset(&if_mac, 0, sizeof(struct ifreq));
	strncpy(if_mac.ifr_name, ifName, IFNAMSIZ-1);
	if (ioctl(sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
		perror("SIOCGIFHWADDR");
	}

	/* Construct the Ethernet header */
	memset(sendbuf, 0, BUF_SIZE);

	/* Ethernet header */
	eh->ether_shost[0] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[0];
	eh->ether_shost[1] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[1];
	eh->ether_shost[2] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[2];
	eh->ether_shost[3] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[3];
	eh->ether_shost[4] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[4];
	eh->ether_shost[5] = ((uint8_t *)&if_mac.ifr_hwaddr.sa_data)[5];
	eh->ether_dhost[0] = DEST_MAC0;
	eh->ether_dhost[1] = DEST_MAC1;
	eh->ether_dhost[2] = DEST_MAC2;
	eh->ether_dhost[3] = DEST_MAC3;
	eh->ether_dhost[4] = DEST_MAC4;
	eh->ether_dhost[5] = macaddr_lastbyte;

	/* EtherType 0x88FB reserved for the PRP protocol */
	eh->ether_type = htons(0x88FB);

	while (1) {
		tx_len = sizeof(struct ether_header);

		/* Packet data */
		sendbuf[tx_len++] = 0x00; /* SupPath */
		sendbuf[tx_len++] = 0x01; /* SupVersion */
		sendbuf[tx_len++] = (seqnum>>8) & 0xff; /* SupSequenceNumber # H */
		sendbuf[tx_len++] =  seqnum & 0xff; /* SupSequenceNumber # L */
		if (mode == 0) {
			sendbuf[tx_len++] = 0x14; /* TLV Type */
		} else {
			sendbuf[tx_len++] = 0x17; /* TLV Type */
		}
		sendbuf[tx_len++] = 0x06; /* TLV Length */
		sendbuf[tx_len++] = eh->ether_shost[0]; /* MAC address of the DANP */
		sendbuf[tx_len++] = eh->ether_shost[1];
		sendbuf[tx_len++] = eh->ether_shost[2];
		sendbuf[tx_len++] = eh->ether_shost[3];
		sendbuf[tx_len++] = eh->ether_shost[4];
		sendbuf[tx_len++] = eh->ether_shost[5];
		sendbuf[tx_len++] = 0x1e; /* TLV2 Type */
		sendbuf[tx_len++] = 0x06; /* TLV2 Length */
		sendbuf[tx_len++] = eh->ether_shost[0]; /* MAC address of the DANP */
		sendbuf[tx_len++] = eh->ether_shost[1];
		sendbuf[tx_len++] = eh->ether_shost[2];
		sendbuf[tx_len++] = eh->ether_shost[3];
		sendbuf[tx_len++] = eh->ether_shost[4];
		sendbuf[tx_len++] = eh->ether_shost[5];
		sendbuf[tx_len++] = 0x0; /* TLV0 Type */
		sendbuf[tx_len++] = 0x0; /* TLV0 Length */

		/* Index of the network device */
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		/* Address length*/
		socket_address.sll_halen = ETH_ALEN;
		/* Destination MAC */
		socket_address.sll_addr[0] = DEST_MAC0;
		socket_address.sll_addr[1] = DEST_MAC1;
		socket_address.sll_addr[2] = DEST_MAC2;
		socket_address.sll_addr[3] = DEST_MAC3;
		socket_address.sll_addr[4] = DEST_MAC4;
		socket_address.sll_addr[5] = macaddr_lastbyte;

		/* Send packet */
		if (sendto(sockfd, sendbuf, tx_len, 0,
			(struct sockaddr*)&socket_address,
			sizeof(struct sockaddr_ll)) < 0) {
			printf("Send failed\n");
		}

		seqnum++;
		seqnum &= 0xffff;

		sleep(polling_sec);
	}

	return 0;
}

/*
 * Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 *
 * This progarm use to get Moxa HSR/PRP Card status.
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

#define IPC_SOCKET_PATH	"\0mxprpipc"
#define BUFFER_SIZE	4096

extern int optind, opterr, optopt;
extern char *optarg;

void usage(void)
{
	printf("Usage:\n");
	printf("\t-h: Show this information.\n");
	printf("\t-l: Show link status.\n");
	printf("\t-s: Show link speed.\n");
	printf("\t-c: Show counter.\n");
	printf("\t-m: Show current mode. 0 is PRP, 1 is HSR.\n");
	printf("\t-p: Set current mode. 0 is PRP, 1 is HSR.\n");
	printf("\t-i: Specify card index, the range is 0~7\n");
	printf("\n\n");
}


int main(int argc, char *argv[]) {
	char *socket_path = IPC_SOCKET_PATH;
	struct sockaddr_un addr;
	char *buf;
	char wbuf[80];
	int fd;
	int rc;
	int cl;
	fd_set set;
	struct timeval timeout;
	char c;
	char optstring[] = "hlscmp:i:";
	int show_link = 0;
	int show_speed = 0;
	int show_counter = 0;
	int show_mode = 0;
	int set_mode = 0;
	int mode = 0;
	int card_index = -1;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			return 0;
		case 'l':
			show_link = 1;
			break;
		case 's':
			show_speed = 1;
			break;
		case 'c':
			show_counter = 1;
			break;
		case 'm':
			show_mode = 1;
			break;
		case 'p':
			set_mode = 1;
			mode = atoi(optarg);
			if ((mode < 0) && (mode > 1)) {
				fprintf(stderr, "Bad argument!\n");
				exit(-1);
			}
			break;
		case 'i':
			card_index = atoi(optarg);
			break;
		}
	}

	if ((show_link == 0) && (show_speed == 0) && (show_counter == 0) &&
		(show_mode == 0) && (set_mode == 0)) {
		show_link = 1;
		show_speed = 1;
		show_counter = 1;
		show_mode = 1;
	}

	if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket error");
		exit(-1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	if (*socket_path == '\0') {
		*addr.sun_path = '\0';
		strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
	} else {
		strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
	}

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("connect error");
		exit(-1);
	}

	buf = malloc(BUFFER_SIZE);

	if (show_mode) {
		char *cmd = "get_prp_mode";
		if (card_index == -1) {
			strcpy(wbuf, cmd);
		} else {
			sprintf(wbuf, "%s %d", cmd, card_index);
		}
		write(fd, wbuf, strlen(wbuf));
		rc = read( fd, buf, BUFFER_SIZE-1 );
		buf[rc] = '\0';
		if (buf[0] != 0) {
			printf("Mode:\n%s\n", buf);
		} else {
			fprintf(stderr, "Device not found!\n");
			exit(-1);
		}
	}

	if (show_link) {
		char *cmd = "get_link_status";
		if (card_index == -1) {
			strcpy(wbuf, cmd);
		} else {
			sprintf(wbuf, "%s %d", cmd, card_index);
		}
		write(fd, wbuf, strlen(wbuf));
		rc = read( fd, buf, BUFFER_SIZE-1 );
		buf[rc] = '\0';
		if (buf[0] != 0) {
			printf("Link Status:\n%s\n", buf);
		} else {
			fprintf(stderr, "Device not found!\n");
			exit(-1);
		}
	}

	if (show_speed) {
		char *cmd = "get_link_speed";
		if (card_index == -1) {
			strcpy(wbuf, cmd);
		} else {
			sprintf(wbuf, "%s %d", cmd, card_index);
		}
		write(fd, wbuf, strlen(wbuf));
		rc = read( fd, buf, BUFFER_SIZE-1 );
		buf[rc] = '\0';
		if (buf[0] != 0) {
			printf("Link Speed:\n%s\n", buf);
		} else {
			fprintf(stderr, "Device not found!\n");
			exit(-1);
		}
	}

	if (show_counter) {
		char *cmd = "get_counters";
		if (card_index == -1) {
			strcpy(wbuf, cmd);
		} else {
			sprintf(wbuf, "%s %d", cmd, card_index);
		}
		write(fd, wbuf, strlen(wbuf));
		rc = read( fd, buf, BUFFER_SIZE-1 );
		buf[rc] = '\0';
		if (buf[0] != 0) {
			printf("Counters:\n%s\n", buf);
		} else {
			fprintf(stderr, "Device not found!\n");
			exit(-1);
		}
	}

	if (set_mode) {
		if (mode == 0) {
			char *cmd = "set_prp_mode";
			if (card_index == -1) {
				strcpy(wbuf, cmd);
			} else {
				sprintf(wbuf, "%s %d", cmd, card_index);
			}
		} else if (mode == 1) {
			char *cmd = "set_hsr_mode";
			if (card_index == -1) {
				strcpy(wbuf, cmd);
			} else {
				sprintf(wbuf, "%s %d", cmd, card_index);
			}
		}
		write(fd, wbuf, strlen(wbuf));
		rc = read( fd, buf, BUFFER_SIZE-1 );
		buf[rc] = '\0';
		if (buf[0] != 0) {
			printf("Set HSR/PRP mode:\n%s\n", buf);
		} else {
			fprintf(stderr, "Device not found!\n");
			exit(-1);
		}
	}

	sprintf(wbuf, "disconnect");
	write(fd, wbuf, strlen(wbuf));

	free(buf);
	write(fd, NULL, 0);
	usleep(100000);
	close(fd);

	return 0;
}

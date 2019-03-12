/*
 * Copyright (C) MOXA Inc. All rights reserved.
 *
 * This software is distributed under the terms of the
 * MOXA License.  See the file COPYING-MOXA for details.
 *
 * Moxa HSR/PRP Card utility
 *
 * 2017-09-18 Holsety Chen
 *   new release
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <linux/i2c-dev.h>
#include "mxhsrprp.h"

#define DEFAULT_PRP_UPDATE_PERIOD_SEC	2
#define DEFAULT_SMBUS_DEV_PATH		"/dev/i2c-0"
#define PIDFILE				"/var/run/mxhsrprpd.pid"
#define ALARM_EXEC_FILE			"/usr/local/bin/mxprpalarm"

/* Used to control the daemon running. 0 for running, else for running */
int bStopping = 0;

struct mxprpdev {
	int fd;
	int mode;
	int new_mode;

	__u8 fpga_major_version;
	__u8 fpga_minor_version;
	struct prp_counters counters_i;
	struct prp_counters counters_a;
	struct prp_counters counters_b;
	int link_speed_i;
	int link_speed_a;
	int link_speed_b;
	int link_status_i;
	int link_status_a;
	int link_status_b;
};

struct mxhsrprp_mgr {
	int quit_flag;
	int polling_sec;
	struct mxprpdev devs[MAX_PRPHSR_CARD];
};

static void *prpmgr_thread(void *arg)
{
	int i;
	struct mxhsrprp_mgr *mgr = arg;
	int prev_link_i[MAX_PRPHSR_CARD];
	int prev_link_a[MAX_PRPHSR_CARD];
	int prev_link_b[MAX_PRPHSR_CARD];
	char str[256];

	/* Configure default mode */
	for (i = 0; i < MAX_PRPHSR_CARD; i++) {
		int fd = mgr->devs[i].fd;
		if (init_card(fd, i) < 0) {
			close(fd);
			mgr->devs[i].fd = -1;
			continue;
		}

		set_prp_mode(fd, mgr->devs[i].mode);
		get_link_status(fd, PORT_I,
			&mgr->devs[i].link_status_i);
		get_link_status(fd, PORT_A,
			&mgr->devs[i].link_status_a);
		get_link_status(fd, PORT_B,
			&mgr->devs[i].link_status_b);
		prev_link_i[i] = mgr->devs[i].link_status_i;
		prev_link_a[i] = mgr->devs[i].link_status_a;
		prev_link_b[i] = mgr->devs[i].link_status_b;
	}

	sleep(1); /* Wait every thing done... */

	while(!mgr->quit_flag)
	{
		for (i = 0; i < MAX_PRPHSR_CARD; i++) {
			struct prp_counters counters;
			int fd = mgr->devs[i].fd;
			if (fd < 0) {
				continue;
			}

			if (mgr->devs[i].mode != mgr->devs[i].new_mode) {
				set_prp_mode(fd, mgr->devs[i].new_mode);
				mgr->devs[i].mode = mgr->devs[i].new_mode;
				memset(&mgr->devs[i].counters_i, 0,
					sizeof(mgr->devs[i].counters_i));
				memset(&mgr->devs[i].counters_a, 0,
					sizeof(mgr->devs[i].counters_a));
				memset(&mgr->devs[i].counters_b, 0,
					sizeof(mgr->devs[i].counters_b));
				continue;
			}

			get_link_status(fd, PORT_I,
				&mgr->devs[i].link_status_i);
			if (mgr->devs[i].link_status_i) {
				get_link_speed(fd, PORT_I,
					&mgr->devs[i].link_speed_i);
				get_prp_counters(fd, PORT_I,
					&mgr->devs[i].counters_i);
			}
			if ( mgr->devs[i].link_status_i != prev_link_i[i] ) {
				prev_link_i[i] = mgr->devs[i].link_status_i;
				sprintf(str, "%s %d l`i %d &", ALARM_EXEC_FILE,
					i, mgr->devs[i].link_status_i);
				system(str);
			}

			get_link_status(fd, PORT_A,
				&mgr->devs[i].link_status_a);
			if (mgr->devs[i].link_status_a) {
				get_link_speed(fd, PORT_A,
					&mgr->devs[i].link_speed_a);
				get_prp_counters(fd, PORT_A,
					&mgr->devs[i].counters_a);
			}
			if ( mgr->devs[i].link_status_a != prev_link_a[i] ) {
				prev_link_a[i] = mgr->devs[i].link_status_a;
				sprintf(str, "%s %d la %d &", ALARM_EXEC_FILE,
					i, mgr->devs[i].link_status_a);
				system(str);
			}


			get_link_status(fd, PORT_B,
				&mgr->devs[i].link_status_b);
			if (mgr->devs[i].link_status_b) {
				get_link_speed(fd, PORT_B,
					&mgr->devs[i].link_speed_b);
				get_prp_counters(fd, PORT_B,
					&mgr->devs[i].counters_b);
			}
			if ( mgr->devs[i].link_status_b != prev_link_b[i] ) {
				prev_link_b[i] = mgr->devs[i].link_status_b;
				sprintf(str, "%s %d lb %d &", ALARM_EXEC_FILE,
					i, mgr->devs[i].link_status_b);
				system(str);
			}
		}

		sleep(mgr->polling_sec);
	}

	for (i = 0; i < MAX_PRPHSR_CARD; i++) {
		if (mgr->devs[i].fd < 0) {
			continue;
		}
		close(mgr->devs[i].fd);
	}

	return 0;
}

#define IPC_SOCKET_PATH	"\0mxprpipc"
static void *prp_status_thread(void *arg)
{
	char *socket_path = IPC_SOCKET_PATH;
	int i;
	struct mxhsrprp_mgr *mgr = arg;
	struct sockaddr_un addr;
	char buf[128];
	char *wbuf;
	int fd;
	int cl;
	int rc;
	fd_set set;
	struct timeval timeout;

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
		unlink(socket_path);
	}

	if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		perror("bind error");
		exit(-1);
	}

	if (listen(fd, 20) == -1) {
		perror("listen error");
		exit(-1);
	}

	wbuf = malloc(4096);
	while (!mgr->quit_flag) {
		FD_ZERO(&set);
		FD_SET(fd, &set);
		timeout.tv_sec = 5;
		timeout.tv_usec = 0;

		rc=select(fd+1,&set, NULL, NULL, &timeout);
		if ( rc == 0 )
			continue;
		else if (rc < 0)
			break;

		if ( (cl = accept(fd, NULL, NULL)) == -1) {
			perror("accept error");
			continue;
		}

		FD_ZERO(&set);
		FD_SET(cl, &set);
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		while ( (rc=select(cl+1,&set, NULL, NULL, &timeout)) > 0) {
			char *token;
			char *parsing[4];
			char *p;
			char str[80];
			int argcnt;
			int stidx = 0;
			int count = MAX_PRPHSR_CARD;

			rc = read( cl, buf, sizeof(buf)-1 );
			if ( rc <= 0 ) {
				break;
			}
			buf[rc] = '\0';
			printf("read %u bytes: %.*s\n", rc, rc, buf);

			argcnt = 0;
			for (argcnt=0, token = strtok(buf, " "); 
				token != NULL && argcnt<4; 
				token = strtok(NULL, " "), argcnt++) {
				parsing[argcnt] = token;
			}

			if (argcnt >=2 ) {
				stidx = atoi(parsing[1]);
				count = 1;
			}

			if (0 == strcmp(parsing[0], "get_link_status")) {
			 	for (i=0; i<count; i++) {
					if ( mgr->devs[stidx+i].fd < 0)
						continue;

					wbuf[0] = '\0';
					p = wbuf;
					p += sprintf(p, "index:%d\n", stidx+i);
					p += sprintf(p, "link_status_i:%d\n", mgr->devs[stidx+i].link_status_i);
					p += sprintf(p, "link_status_a:%d\n", mgr->devs[stidx+i].link_status_a);
					p += sprintf(p, "link_status_b:%d\n", mgr->devs[stidx+i].link_status_b);
					write(cl, wbuf, strlen(wbuf));
				}
			} else if (0 == strcmp( parsing[0], "get_link_speed") ) {
			 	for (i=0; i<count; i++) {
					if ( mgr->devs[stidx+i].fd < 0)
						continue;

					wbuf[0] = '\0';
					p = wbuf;
					p += sprintf(p, "index:%d\n", stidx+i);
					p += sprintf(p, "link_speed_i:%d\n", mgr->devs[stidx+i].link_speed_i);
					p += sprintf(p, "link_speed_a:%d\n", mgr->devs[stidx+i].link_speed_a);
					p += sprintf(p, "link_speed_b:%d\n", mgr->devs[stidx+i].link_speed_b);
					write(cl, wbuf, strlen(wbuf));
				}
			} else if (0 == strcmp( parsing[0], "get_counters") ) {
			 	for (i=0; i<count; i++) {
					if ( mgr->devs[stidx+i].fd < 0)
						continue;

					wbuf[0] = '\0';
					p = wbuf;
					p += sprintf(p, "index:%d\n", stidx+i);
					p += sprintf(p, "\nport: interlink\n");
					p += sprintf(p, "rx_good_octets:%ld\n", mgr->devs[stidx+i].counters_i.rx_good_octets);
					p += sprintf(p, "rx_bad_octets:%ld\n", mgr->devs[stidx+i].counters_i.rx_bad_octets);
					p += sprintf(p, "rx_unicast:%ld\n", mgr->devs[stidx+i].counters_i.rx_unicast);
					p += sprintf(p, "rx_broadcast:%ld\n", mgr->devs[stidx+i].counters_i.rx_broadcast);
					p += sprintf(p, "rx_multicast:%ld\n", mgr->devs[stidx+i].counters_i.rx_multicast);
					p += sprintf(p, "rx_undersize:%ld\n", mgr->devs[stidx+i].counters_i.rx_undersize);
					p += sprintf(p, "rx_fragments:%ld\n", mgr->devs[stidx+i].counters_i.rx_fragments);
					p += sprintf(p, "rx_oversize:%ld\n", mgr->devs[stidx+i].counters_i.rx_oversize);
					p += sprintf(p, "rx_jabber:%ld\n", mgr->devs[stidx+i].counters_i.rx_jabber);
					p += sprintf(p, "rx_err:%ld\n", mgr->devs[stidx+i].counters_i.rx_err);
					p += sprintf(p, "rx_crc:%ld\n", mgr->devs[stidx+i].counters_i.rx_crc);
					p += sprintf(p, "rx_64:%ld\n", mgr->devs[stidx+i].counters_i.rx_64);
					p += sprintf(p, "rx_65_127:%ld\n", mgr->devs[stidx+i].counters_i.rx_65_127);
					p += sprintf(p, "rx_128_255:%ld\n", mgr->devs[stidx+i].counters_i.rx_128_255);
					p += sprintf(p, "rx_256_511:%ld\n", mgr->devs[stidx+i].counters_i.rx_256_511);
					p += sprintf(p, "rx_512_1023:%ld\n", mgr->devs[stidx+i].counters_i.rx_512_1023);
					p += sprintf(p, "rx_1024_1536:%ld\n", mgr->devs[stidx+i].counters_i.rx_1024_1536);
					p += sprintf(p, "rx_hsrprp:%ld\n", mgr->devs[stidx+i].counters_i.rx_hsrprp);
					p += sprintf(p, "rx_wronglan:%ld\n", mgr->devs[stidx+i].counters_i.rx_wronglan);
					p += sprintf(p, "rx_duplicate:%ld\n", mgr->devs[stidx+i].counters_i.rx_duplicate);
					p += sprintf(p, "tx_octets:%ld\n", mgr->devs[stidx+i].counters_i.tx_octets);
					p += sprintf(p, "tx_unicast:%ld\n", mgr->devs[stidx+i].counters_i.tx_unicast);
					p += sprintf(p, "tx_broadcast:%ld\n", mgr->devs[stidx+i].counters_i.tx_broadcast);
					p += sprintf(p, "tx_multicast:%ld\n", mgr->devs[stidx+i].counters_i.tx_multicast);
					p += sprintf(p, "tx_hsrprp:%ld\n", mgr->devs[stidx+i].counters_i.tx_hsrprp);
					p += sprintf(p, "priq_drop:%ld\n", mgr->devs[stidx+i].counters_i.priq_drop);
					p += sprintf(p, "early_drop:%ld\n", mgr->devs[stidx+i].counters_i.early_drop);
					p += sprintf(p, "\nport: lana\n");
					p += sprintf(p, "rx_good_octets:%ld\n", mgr->devs[stidx+i].counters_a.rx_good_octets);
					p += sprintf(p, "rx_bad_octets:%ld\n", mgr->devs[stidx+i].counters_a.rx_bad_octets);
					p += sprintf(p, "rx_unicast:%ld\n", mgr->devs[stidx+i].counters_a.rx_unicast);
					p += sprintf(p, "rx_broadcast:%ld\n", mgr->devs[stidx+i].counters_a.rx_broadcast);
					p += sprintf(p, "rx_multicast:%ld\n", mgr->devs[stidx+i].counters_a.rx_multicast);
					p += sprintf(p, "rx_undersize:%ld\n", mgr->devs[stidx+i].counters_a.rx_undersize);
					p += sprintf(p, "rx_fragments:%ld\n", mgr->devs[stidx+i].counters_a.rx_fragments);
					p += sprintf(p, "rx_oversize:%ld\n", mgr->devs[stidx+i].counters_a.rx_oversize);
					p += sprintf(p, "rx_jabber:%ld\n", mgr->devs[stidx+i].counters_a.rx_jabber);
					p += sprintf(p, "rx_err:%ld\n", mgr->devs[stidx+i].counters_a.rx_err);
					p += sprintf(p, "rx_crc:%ld\n", mgr->devs[stidx+i].counters_a.rx_crc);
					p += sprintf(p, "rx_64:%ld\n", mgr->devs[stidx+i].counters_a.rx_64);
					p += sprintf(p, "rx_65_127:%ld\n", mgr->devs[stidx+i].counters_a.rx_65_127);
					p += sprintf(p, "rx_128_255:%ld\n", mgr->devs[stidx+i].counters_a.rx_128_255);
					p += sprintf(p, "rx_256_511:%ld\n", mgr->devs[stidx+i].counters_a.rx_256_511);
					p += sprintf(p, "rx_512_1023:%ld\n", mgr->devs[stidx+i].counters_a.rx_512_1023);
					p += sprintf(p, "rx_1024_1536:%ld\n", mgr->devs[stidx+i].counters_a.rx_1024_1536);
					p += sprintf(p, "rx_hsrprp:%ld\n", mgr->devs[stidx+i].counters_a.rx_hsrprp);
					p += sprintf(p, "rx_wronglan:%ld\n", mgr->devs[stidx+i].counters_a.rx_wronglan);
					p += sprintf(p, "rx_duplicate:%ld\n", mgr->devs[stidx+i].counters_a.rx_duplicate);
					p += sprintf(p, "tx_octets:%ld\n", mgr->devs[stidx+i].counters_a.tx_octets);
					p += sprintf(p, "tx_unicast:%ld\n", mgr->devs[stidx+i].counters_a.tx_unicast);
					p += sprintf(p, "tx_broadcast:%ld\n", mgr->devs[stidx+i].counters_a.tx_broadcast);
					p += sprintf(p, "tx_multicast:%ld\n", mgr->devs[stidx+i].counters_a.tx_multicast);
					p += sprintf(p, "tx_hsrprp:%ld\n", mgr->devs[stidx+i].counters_a.tx_hsrprp);
					p += sprintf(p, "priq_drop:%ld\n", mgr->devs[stidx+i].counters_a.priq_drop);
					p += sprintf(p, "early_drop:%ld\n", mgr->devs[stidx+i].counters_a.early_drop);
					p += sprintf(p, "\nport: lanb\n");
					p += sprintf(p, "rx_good_octets:%ld\n", mgr->devs[stidx+i].counters_b.rx_good_octets);
					p += sprintf(p, "rx_bad_octets:%ld\n", mgr->devs[stidx+i].counters_b.rx_bad_octets);
					p += sprintf(p, "rx_unicast:%ld\n", mgr->devs[stidx+i].counters_b.rx_unicast);
					p += sprintf(p, "rx_broadcast:%ld\n", mgr->devs[stidx+i].counters_b.rx_broadcast);
					p += sprintf(p, "rx_multicast:%ld\n", mgr->devs[stidx+i].counters_b.rx_multicast);
					p += sprintf(p, "rx_undersize:%ld\n", mgr->devs[stidx+i].counters_b.rx_undersize);
					p += sprintf(p, "rx_fragments:%ld\n", mgr->devs[stidx+i].counters_b.rx_fragments);
					p += sprintf(p, "rx_oversize:%ld\n", mgr->devs[stidx+i].counters_b.rx_oversize);
					p += sprintf(p, "rx_jabber:%ld\n", mgr->devs[stidx+i].counters_b.rx_jabber);
					p += sprintf(p, "rx_err:%ld\n", mgr->devs[stidx+i].counters_b.rx_err);
					p += sprintf(p, "rx_crc:%ld\n", mgr->devs[stidx+i].counters_b.rx_crc);
					p += sprintf(p, "rx_64:%ld\n", mgr->devs[stidx+i].counters_b.rx_64);
					p += sprintf(p, "rx_65_127:%ld\n", mgr->devs[stidx+i].counters_b.rx_65_127);
					p += sprintf(p, "rx_128_255:%ld\n", mgr->devs[stidx+i].counters_b.rx_128_255);
					p += sprintf(p, "rx_256_511:%ld\n", mgr->devs[stidx+i].counters_b.rx_256_511);
					p += sprintf(p, "rx_512_1023:%ld\n", mgr->devs[stidx+i].counters_b.rx_512_1023);
					p += sprintf(p, "rx_1024_1536:%ld\n", mgr->devs[stidx+i].counters_b.rx_1024_1536);
					p += sprintf(p, "rx_hsrprp:%ld\n", mgr->devs[stidx+i].counters_b.rx_hsrprp);
					p += sprintf(p, "rx_wronglan:%ld\n", mgr->devs[stidx+i].counters_b.rx_wronglan);
					p += sprintf(p, "rx_duplicate:%ld\n", mgr->devs[stidx+i].counters_b.rx_duplicate);
					p += sprintf(p, "tx_octets:%ld\n", mgr->devs[stidx+i].counters_b.tx_octets);
					p += sprintf(p, "tx_unicast:%ld\n", mgr->devs[stidx+i].counters_b.tx_unicast);
					p += sprintf(p, "tx_broadcast:%ld\n", mgr->devs[stidx+i].counters_b.tx_broadcast);
					p += sprintf(p, "tx_multicast:%ld\n", mgr->devs[stidx+i].counters_b.tx_multicast);
					p += sprintf(p, "tx_hsrprp:%ld\n", mgr->devs[stidx+i].counters_b.tx_hsrprp);
					p += sprintf(p, "priq_drop:%ld\n", mgr->devs[stidx+i].counters_b.priq_drop);
					p += sprintf(p, "early_drop:%ld\n", mgr->devs[stidx+i].counters_b.early_drop);
					write(cl, wbuf, strlen(wbuf));
				}
			} else if (0 == strcmp( parsing[0], "get_prp_mode") ) {
			 	for (i=0; i<count; i++) {
					if (mgr->devs[stidx+i].fd < 0)
						continue;

					wbuf[0] = '\0';
					p = wbuf;
					p += sprintf(p, "index:%d\n", stidx+i);
					p += sprintf(p, "mode:%d\n", mgr->devs[stidx+i].mode);
					write(cl, wbuf, strlen(wbuf));
				}
			} else if (0 == strcmp( parsing[0], "set_prp_mode") ) {
			 	for (i=0; i<count; i++) {
					if (mgr->devs[stidx+i].fd < 0)
						continue;

					mgr->devs[stidx+i].new_mode = MODE_PRP;

					wbuf[0] = '\0';
					p = wbuf;
					p += sprintf(p, "index:%d, curr: %d, new:%d\n",
						stidx+i,
						mgr->devs[stidx+i].mode,
						mgr->devs[stidx+i].new_mode);
					write(cl, wbuf, strlen(wbuf));
				}
			} else if (0 == strcmp( parsing[0], "set_hsr_mode") ) {
			 	for (i=0; i<count; i++) {
					if (mgr->devs[stidx+i].fd < 0)
						continue;

					mgr->devs[stidx+i].new_mode = MODE_HSR;

					wbuf[0] = '\0';
					p = wbuf;
					p += sprintf(p, "index:%d, curr: %d, new:%d\n",
						stidx+i,
						mgr->devs[stidx+i].mode,
						mgr->devs[stidx+i].new_mode);
					write(cl, wbuf, strlen(wbuf));
				}
			} else if (0 == strcmp( parsing[0], "disconnect") ) {
				close(cl);
				break;
			} else {
			}

			/* write EOF */
			wbuf[0] = '\0';
			write(cl, wbuf, 1);
		}
		if (rc == -1) {
			perror("read error");
			close(cl);
		}
		else if (rc == 0) {
			close(cl);
		}
	}

	close(fd);

	free(wbuf);
}

extern int optind, opterr, optopt;
extern char *optarg;

int parsing_string( char* source, char **split, char *key, int count )
{
	char *pch = strtok(source, key );
	int i;

	for (i = 0; (pch != NULL) && (i < count); i++)
	{
		split[i] = pch;
		pch = strtok(NULL, key);
	}

	return i;
}


int remove_pid_file(const char *pidFile) {

	bStopping = 1;

	if ( unlink(pidFile) < 0 ) {
		printf("unlink(%s) fail\n", pidFile);
		return -EFAULT;
	}

	return 0;
}

int create_pid_file(const char *pidFile) {
	char buf[20];
	int fd_pid;

	fd_pid = open(pidFile, O_CREAT|O_TRUNC|O_RDWR);
	if ( fd_pid < 0 ) {
		printf("Open %s fail\n", pidFile);
		return -EEXIST;
	}
	sprintf(buf, "%d", getpid());
	if(write(fd_pid, buf, sizeof(buf)) < 0 ) {
		printf("Write %s fail\n", pidFile);
		return -EACCES;
	}

	close(fd_pid);

	return 0;
}

void sig_handler_for_stopping_process(int sig) {

	remove_pid_file(PIDFILE);
}

void usage(void)
{
	printf("Usage:\n");
	printf("\t-h: Show this information.\n");
	printf("\t-B: Run daemon in the background\n");
	printf("\t-b: SMBUS device, default is /dev/i2c-0\n");
	printf("\t-t: HSR/PRP Status update period. Default is %d second.\n",
		DEFAULT_PRP_UPDATE_PERIOD_SEC);
	printf("\t-m: configure to prp or hsr mode, default is prp mode.\n");
	printf("\t\tThe argurement is [index]:[mode]\n");
	printf("\t\tindex range from 0~7, mode 0 is prp, mode 1 is hsr.\n");
	printf("\t\tEx: Set card 0 to hsr mode, card 1 to prp mode.");
	printf("\t\t\t root@Moxa:~# mxhsrprpd -t 2 -m 0:1,1:0");
	printf("\n\n");
}

int main(int argc, char *argv[]) {
	int i;
	int be_a_daemon = 0;
	char c;
	char optstring[] = "hBb:t:m:";
	char smbus_dev[260] = DEFAULT_SMBUS_DEV_PATH;
	char str[20];
	struct mxhsrprp_mgr prp_mgr;
	pthread_t prpmgr_thread_id;
	pthread_t prp_status_thread_id;

	memset(&prp_mgr, 0, sizeof(prp_mgr));
	prp_mgr.polling_sec = DEFAULT_PRP_UPDATE_PERIOD_SEC;

	while ((c = getopt(argc, argv, optstring)) != -1) {
		switch (c) {
		case 'h':
		case '?':
			usage();
			return 0;
		case 'B':
			be_a_daemon = 1;
			break;
		case 'b':
			strcpy(smbus_dev, optarg);
			break;
		case 't':
			prp_mgr.polling_sec = atoi(optarg);
			if ( prp_mgr.polling_sec < 0) {
				fprintf(stderr, "Bad argument!\n");
				exit(-1);
			}
			break;
		case 'm':
			{
			char params[80];
			char *p[8];
			int param_count = 0;

			strcpy(params, optarg);
			param_count = parsing_string(params, p, ",", 8);
			for (i = 0; i < param_count; i++) {
				char *p2[2];
				if (2 == parsing_string(p[i], p2, ":", 2)) {
					int idx = atoi(p2[0]);
					int mode = atoi(p2[1]);
					if ((idx < 0) ||
					    (idx >= MAX_PRPHSR_CARD) ||
					    (mode < 0) || (mode > 1) ) {
						fprintf(stderr,
							"Bad argument!\n");
						exit(-1);
					}
					prp_mgr.devs[idx].mode = mode;
					prp_mgr.devs[idx].new_mode = mode;
				}
			}
			}
			break;
		}
	}

	/* To run in Daemon mode */
	if ( be_a_daemon ) {
		int pid;
		int remove_files_signal_list[] = {
			SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGABRT, SIGFPE,
			SIGKILL, SIGSEGV, SIGPIPE, SIGALRM, SIGTERM, SIGUSR1,
			SIGUSR2};
		pid = fork();
		if ( pid < 0 ) {
			return -1;
		}
		else if ( pid > 0 ) {
			exit(0);
		}

		/* mew session founder process */
		setsid();
		pid = fork();
		if ( pid < 0 ) {
			return -1;
		}
		else if ( pid > 0 ) {
			exit(0);
		}

		/* child process */
		chdir("/");
		umask(0);

		/* Register the signal handler for the pid file removal */
		for( i=0; i < sizeof(remove_files_signal_list)/sizeof(int); i++ )
			signal(remove_files_signal_list[i], sig_handler_for_stopping_process);

		/* Create the child process pid file */
		create_pid_file(PIDFILE);
	}

	/* Get version to make sure the device is avaliable */
	for (i = 0; i < MAX_PRPHSR_CARD; i++) {
		int fd = open(smbus_dev, O_RDWR);

		if (fd < 0 ) {
			fprintf(stderr, "Open %s fail!\n", smbus_dev);
			exit(-1);
		}
		prp_mgr.devs[i].fd = fd;
	}

	prp_mgr.quit_flag = 0;

	pthread_create(&prpmgr_thread_id, NULL, prpmgr_thread, &prp_mgr);
	pthread_create(&prp_status_thread_id, NULL, prp_status_thread, &prp_mgr);

	if (be_a_daemon) {
		while ( !bStopping ) {
			sleep(1);
		}
	} else {
		/* wait for key in quit*/
		while(1) {
			fgets(str, sizeof(str), stdin);
			if (0 == strcmp("quit\n", str)) {
				break;
			}
		}
	}

	prp_mgr.quit_flag = 1;
	pthread_join(prp_status_thread_id, NULL);
	pthread_join(prpmgr_thread_id, NULL);

	return 0;
}

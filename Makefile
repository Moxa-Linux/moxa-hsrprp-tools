DESTDIR ?= /
CC=gcc
CFLAGS = -DPRPLED_HA

all:
	@$(CC) $(CFLAGS) mxhsrprpd.c mxhsrprp.c -lpthread -o mxhsrprpd
	@$(CC) mxprpinfo.c -o mxprpinfo
	@$(CC) prpsuper.c -o mxprpsuper

install:
	mkdir -p $(DESTDIR)/usr/sbin/
	cp mxhsrprpd $(DESTDIR)/usr/sbin
	cp mxprpinfo $(DESTDIR)/usr/sbin
	cp mxprpsuper $(DESTDIR)/usr/sbin

clean:
	@rm -f mxhsrprpd
	@rm -f mxprpinfo
	@rm -f mxprpsuper

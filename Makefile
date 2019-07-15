CC=gcc
STRIP=strip
LFLAGS=-fPIC -shared

CFLAGS = -DPRPLED_HA

all:
	@$(CC) $(CFLAGS) mxhsrprpd.c mxhsrprp.c -lpthread -o mxhsrprpd
	@$(CC) mxprpinfo.c -o mxprpinfo
	@$(CC) prpsuper.c -o mxprpsuper

clean:
	@rm -f mxhsrprpd
	@rm -f mxprpinfo
	@rm -f mxprpsuper

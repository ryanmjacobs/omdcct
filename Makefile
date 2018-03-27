CC?=gcc
CFLAGS=-Wall -Wextra -m64 -std=gnu99 -O2 -march=native\
	   -D_FILE_OFFSET_BITS=64
LDFLAGS=-lpthread

all: plot mine mine_pool_all mine_pool_share

plot: plot.c shabal64.o helper64.o mshabal_sse2.o nonce.o opts.o
	$(CC) $(CFLAGS) -o plot plot.c shabal64.o helper64.o mshabal_sse2.o\
		nonce.o opts.o $(LDFLAGS)

mine: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) -DSOLO -o mine mine.c shabal64.o helper64.o $(LDFLAGS)

mine_pool_all: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) -DURAY_POOL -o mine_pool_all mine.c shabal64.o helper64.o $(LDFLAGS)

mine_pool_share: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) -DSHARE_POOL -o mine_pool_share mine.c shabal64.o helper64.o $(LDFLAGS)

nonce.o: nonce.c
	$(CC) $(CFLAGS) -c -o nonce.o nonce.c

opts.o: opts.c
	$(CC) $(CFLAGS) -c -o opts.o opts.c

helper64.o: helper.c
	$(CC) $(CFLAGS) -c -o helper64.o helper.c

shabal64.o: shabal64.s
	$(CC) $(CFLAGS) -c -o shabal64.o shabal64.s

clean:
	rm -rf *.o
	rm -rf 123_*
	rm -rf plot mine mine_pool mine_pool_all

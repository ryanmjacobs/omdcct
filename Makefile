CC?=gcc
CFLAGS=-Wall -m64 -std=gnu99 -O2 -march=native\
	   -D_FILE_OFFSET_BITS=64
LDFLAGS=-lpthread

all: plot mine mine_pool_all mine_pool_share

plot: plot.c shabal64.o helper64.o mshabal_sse4.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o plot plot.c shabal64.o helper64.o mshabal_sse4.o

mine: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) $(LDFLAGS) -DSOLO -o mine mine.c shabal64.o helper64.o

mine_pool_all: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) $(LDFLAGS) -DURAY_POOL -o mine_pool_all mine.c shabal64.o helper64.o

mine_pool_share: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) $(LDFLAGS) -DSHARE_POOL -o mine_pool_share mine.c shabal64.o helper64.o

helper64.o: helper.c
	$(CC) $(CFLAGS) -c -o helper64.o helper.c

shabal64.o: shabal64.s
	$(CC) $(CFLAGS) -c -o shabal64.o shabal64.s

clean:
	rm -rf *.o
	rm -rf plot mine mine_pool mine_pool_all

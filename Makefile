CC?=gcc
CFLAGS=-Wall -m64 -std=gnu99 -O2 -march=native\
	   -D_FILE_OFFSET_BITS=64

all: plot optimize mine mine_pool_all mine_pool_share

plot: plot.c shabal64.o helper64.o mshabal_sse4.o
	$(CC) $(CFLAGS) -o plot plot.c shabal64.o helper64.o mshabal_sse4.o -lpthread

mine: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) -DSOLO -o mine mine.c shabal64.o helper64.o -lpthread

mine_pool_all: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) -DURAY_POOL -o mine_pool_all mine.c shabal64.o helper64.o -lpthread

mine_pool_share: mine.c shabal64.o helper64.o
	$(CC) $(CFLAGS) -DSHARE_POOL -o mine_pool_share mine.c shabal64.o helper64.o -lpthread

optimize: optimize.c helper64.o
	$(CC) $(CFLAGS) -o optimize optimize.c helper64.o

helper64.o: helper.c
	$(CC) $(CFLAGS) -c -o helper64.o helper.c

shabal64.o: shabal64.s
	$(CC) $(CFLAGS) -c -o shabal64.o shabal64.s

clean:
	rm -rf *.o
	rm -rf plot mine mine_pool mine_pool_all optimize

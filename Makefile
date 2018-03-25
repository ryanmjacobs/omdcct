CFLAGS=-D_FILE_OFFSET_BITS=64 -std=gnu99

all: plot optimize mine mine_pool_all mine_pool_share

plot: plot.c shabal64.o helper64.o mshabal_sse4.o
	gcc -Wall -m64 -O2 -o plot plot.c shabal64.o helper64.o mshabal_sse4.o -march=native -lpthread

mine: mine.c shabal64.o helper64.o
	gcc -Wall -m64 -O2 -DSOLO -o mine mine.c shabal64.o helper64.o -lpthread

mine_pool_all: mine.c shabal64.o helper64.o
	gcc -Wall -m64 -O2 -DURAY_POOL -o mine_pool_all mine.c shabal64.o helper64.o -lpthread

mine_pool_share: mine.c shabal64.o helper64.o
	gcc -Wall -m64 -O2 -DSHARE_POOL -o mine_pool_share mine.c shabal64.o helper64.o -lpthread

optimize: optimize.c helper64.o
	gcc -Wall -m64 -O2 -o optimize optimize.c helper64.o

helper64.o: helper.c
	gcc -Wall -m64 -c -O2 -o helper64.o helper.c

shabal64.o: shabal64.s
	gcc -Wall -m64 -c -O2 -o shabal64.o shabal64.s

clean:
	rm -rf *.o
	rm -rf plot mine mine_pool mine_pool_all optimize

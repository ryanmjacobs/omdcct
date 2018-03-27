#define _GNU_SOURCE
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "shabal.h"
#include "mshabal.h"
#include "helper.h"
#include "nonce.h"
#include "opts.h"

char *cache;
struct opts_t *opts;

struct worker_args_t {
    unsigned long long i;
    struct opts_t opts;
};

void *work_i(void *x);
unsigned long long get_ms();
void print_stats(struct opts_t o, unsigned long long nr, unsigned long long start_ms);

int main(int argc, char **argv) {
    // plotting code is completely dependent on
    // 'unsigned long long' being 64 bits long
    assert(sizeof(unsigned long long) == 8);

    struct opts_t o = get_opts(argc, argv);
    opts = &o;

    if (sse2_supported() && o.use_sse2)
        printf("Using SSE2 core.\n");
    else
        printf("Using original algorithm.\n");

    // 32 Bit and above 4GB?
    if (sizeof( void* ) < 8 ) {
        if (o.stagger_size > 15000 ) {
            printf("Cant use stagger sizes above 15000 with 32-bit version\n");
            exit(-1);
        }
    }

    // Adjust according to stagger size
    if (o.num_nonces % o.stagger_size != 0) {
        o.num_nonces -= o.num_nonces % o.stagger_size;
        o.num_nonces += o.stagger_size;
        printf("Adjusting total nonces to %llu to match stagger size\n", o.num_nonces);
    }

    printf("Creating plots for nonces %llu to %llu (%u GB) using %u MB memory and %u threads\n",
            o.start_nonce,
            (o.start_nonce + o.num_nonces),
            (unsigned int)(o.num_nonces / 4 / 953),
            (unsigned int)(o.stagger_size / 4),
            o.num_threads);

    cache = calloc(PLOT_SIZE, o.stagger_size);

    if (cache == NULL) {
        printf("Error allocating memory. Try lower stagger size.\n");
        exit(-1);
    }

    // create filename
    char fname[100];
    sprintf(fname, "%llu_%llu_%llu_%llu",
            o.addr, o.start_nonce, o.num_nonces, o.stagger_size);

    // create file
    FILE *fp = fopen(fname, "w");
    if(fp == NULL) {
        printf("error: opening file %s\n", fname);
        exit(-1);
    }

    pthread_t worker[o.num_threads];
    unsigned long long nonceoffset[o.num_threads];

    for (unsigned long long nr = 0; nr < o.num_nonces; nr += o.stagger_size) {
        unsigned long long start_ms = get_ms();

        for (unsigned i = 0; i < o.num_threads; i++) {
            nonceoffset[i] = o.start_nonce + i * o.noncesperthread;

            struct worker_args_t *args = malloc(sizeof(struct worker_args_t));
            args->i = nonceoffset[i];
            args->opts = o;

            if (pthread_create(&worker[i], NULL, work_i, args)) {
                printf("error: unable to create thread. Out of memory? Try lower stagger size / less threads\n");
                exit(-1);
            }
        }

        // wait for threads to finish
        for(unsigned i = 0; i < o.num_threads; i++)
            pthread_join(worker[i], NULL);

        // run leftover nonces
        for (unsigned long long i = o.num_threads * o.noncesperthread; i < o.stagger_size; i++)
            nonce(o.addr, o.start_nonce + i, i);

        // write plot to disk
        print_stats(o, nr, start_ms);
        fwrite(cache, PLOT_SIZE, o.stagger_size, fp);

        o.start_nonce += o.stagger_size;
    }

    fclose(fp);
    printf("\nFinished plotting.\n");
    return 0;
}

void *work_i(void *x) {
    struct worker_args_t *args = x;

    struct opts_t o = args->opts;
    unsigned long long i = args->i;

    for (unsigned long long n = 0; n < o.noncesperthread; n++) {
        if (o.use_sse2) {
            if (n + 4 < o.noncesperthread) {
                mnonce(o.addr,
                      (i + n + 0), (i + n + 1), (i + n + 2), (i + n + 3),
                      (i - o.start_nonce + n + 0),
                      (i - o.start_nonce + n + 1),
                      (i - o.start_nonce + n + 2),
                      (i - o.start_nonce + n + 3));

                n += 3;
            } else {
               nonce(o.addr, (i + n), (i - o.start_nonce + n));
            }
        } else {
            nonce(o.addr, (i + n), (i - o.start_nonce + n));
        }
    }

    return NULL;
}

void print_stats(struct opts_t o, unsigned long long nr, unsigned long long start_ms) {
    // calculate percentage complete and nonces/min
    int percent = (int)(100 * (nr + o.stagger_size) / o.num_nonces);
    double minutes = (double) (get_ms() - start_ms) / (60 * 1000000);
    int speed = (int)(o.stagger_size / minutes);
    int m = (int)(o.num_nonces - nr) / speed;
    int h = (int)(m / 60);
    m -= h * 60;

    // print stats
    printf("\r%i Percent done. %i nonces/minute, %i:%02i left", percent, speed, h, m);
    fflush(stdout);
}

unsigned long long get_ms() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
}


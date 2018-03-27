/*
    Even faster plot generator for burstcoin
    Modified version originally written by Markus Tervooren
    Added code for mshabal/mshabal256 (SSE2 optimizations)
    by Cerr Janror <cerr.janror@gmail.com> : https://github.com/BurstTools/BurstSoftware.git

    Author: Mirkic7 <mirkic7@hotmail.com>
    Burst: BURST-RQW7-3HNW-627D-3GAEV

    Original author: Markus Tervooren <info@bchain.info>
    Burst: BURST-R5LP-KEL9-UYLG-GFG6T

    Implementation of Shabal is taken from:
    http://www.shabal.com/?p=198
*/

#define _GNU_SOURCE
#include <stdint.h>
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

unsigned long long getMS() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
}

void *writecache(FILE *fp, struct opts_t o, unsigned run, unsigned lastrun, unsigned long long starttime) {
    int percent = (int)(100 * lastrun / o.num_nonces);
    printf("\r%i Percent done. (write)", percent);
    fflush(stdout);

    // write cache
    fwrite(cache, PLOT_SIZE, o.stagger_size, fp);
    fclose(fp);

    // calculate stats
    unsigned long long ms = getMS() - starttime;
    double minutes = (double)ms / (1000000 * 60);
    int speed = (int)(o.stagger_size / minutes);
    int m = (int)(o.num_nonces - run) / speed;
    int h = (int)(m / 60);
    m -= h * 60;

    printf("\r%i Percent done. %i nonces/minute, %i:%02i left", percent, speed, h, m);
    fflush(stdout);

    return NULL;
}


int main(int argc, char **argv) {
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

    unsigned lastrun;
    unsigned long long starttime, astarttime;
    for (unsigned run = 0; run < o.num_nonces; run += o.stagger_size) {
        astarttime = getMS();

        for (unsigned i = 0; i < o.num_threads; i++) {
            nonceoffset[i] = o.start_nonce + i * o.noncesperthread;

            struct worker_args_t *args = malloc(sizeof(struct worker_args_t));
            args->i = nonceoffset[i];
            args->opts = o;

            if (pthread_create(&worker[i], NULL, work_i, args)) {
                printf("Error creating thread. Out of memory? Try lower stagger size / less threads\n");
                exit(-1);
            }
        }

        // wait for threads to finish
        for(unsigned i = 0; i < o.num_threads; i++)
            pthread_join(worker[i], NULL);

        // run leftover nonces
        for (unsigned long long i = o.num_threads * o.noncesperthread; i < o.stagger_size; i++)
            nonce(o.addr, o.start_nonce + i, i);

        // Write plot to disk:
        starttime=astarttime;
        lastrun=run+o.stagger_size;
        writecache(fp, o, run, lastrun, starttime);

        o.start_nonce += o.stagger_size;
    }

    printf("\nFinished plotting.\n");
    return 0;
}

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

#define USE_MULTI_SHABAL

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

// Leave 5GB free space
#define FREE_SPACE	(unsigned long long)5 * 1000 * 1000 * 1000

// Not to be changed below this
#define PLOT_SIZE	(4096 * 64)
#define HASH_SIZE	32
#define HASH_CAP	4096

unsigned long long addr = 0;
unsigned long long startnonce = 0;
unsigned int nonces = 0;
unsigned int staggersize = 0;
unsigned int threads = 0;
unsigned int noncesperthread;
unsigned long long starttime;
int ofd, run, lastrun;

char *cache, *wcache, *acache[2];

void *work_i(void *x_void_ptr) {
    unsigned long long *x_ptr = (unsigned long long *)x_void_ptr;
    unsigned long long i = *x_ptr;

    for (int n=0; n<noncesperthread; n++) {
        if (sse2_supported()) {
            if (n + 4 < noncesperthread) {
                mnonce(addr,
                      (i + n + 0), (i + n + 1), (i + n + 2), (i + n + 3),
                      (unsigned long long)(i - startnonce + n + 0),
                      (unsigned long long)(i - startnonce + n + 1),
                      (unsigned long long)(i - startnonce + n + 2),
                      (unsigned long long)(i - startnonce + n + 3));

                n += 3;
            } else {
               nonce(addr,(i + n), (unsigned long long)(i - startnonce + n));
            }
        } else {
            nonce(addr,(i + n), (unsigned long long)(i - startnonce + n));
        }
    }

    return NULL;
}

unsigned long long getMS() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((unsigned long long)time.tv_sec * 1000000) + time.tv_usec;
}

void usage(const char *progname) {
    fprintf(stderr, "Usage: %s -k KEY [-s STARTNONCE] [-n NONCES] [-m STAGGERSIZE] "
                    "[-t THREADS]\n", progname);
    exit(-1);
}

void *writecache(void *arguments) {
    unsigned long long bytes = (unsigned long long) staggersize * PLOT_SIZE;
    unsigned long long position = 0;
    int percent;

    percent = (int)(100 * lastrun / nonces);

    printf("\r%i Percent done. (write)", percent);
    fflush(stdout);

    do {
        // Don't write more than 100MB at once
        int b = write(ofd, &wcache[position], bytes > 100000000 ? 100000000 : bytes);
        position += b;
        bytes -= b;
    } while(bytes > 0);

    unsigned long long ms = getMS() - starttime;

    percent = (int)(100 * lastrun / nonces);
    double minutes = (double)ms / (1000000 * 60);
    int speed = (int)(staggersize / minutes);
    int m = (int)(nonces - run) / speed;
    int h = (int)(m / 60);
    m -= h * 60;

    printf("\r%i Percent done. %i nonces/minute, %i:%02i left", percent, speed, h, m);
    fflush(stdout);

    return NULL;
}

int main(int argc, char **argv) {
    if(argc < 2)
        usage(argv[0]);

    // pick a start nonce,
    // the user may or not override this
    srand(time(NULL));
    startnonce = (unsigned long long)rand() * (1 << 30) + rand();

    for (int i = 1; i < argc; i++) {
        // Ignore unknown argument
        if(argv[i][0] != '-')
            continue;

        // flag must be one character
        // flag must be followed by one value
        if (argv[i][2] != '\0' || i > argc-1)
            usage(argv[0]);

        char flag = argv[i][1];
        char *parse = argv[++i];

        // to-be-parsed value must exist
        if (parse == NULL)
            usage(argv[0]);

        unsigned long long value = strtoull(parse, 0, 10);

        switch (flag) {
            case 'k':
                addr = value;
                break;
            case 's':
                startnonce = value;
                break;
            case 'n':
                nonces = value;
                break;
            case 'm':
                staggersize = value;
                break;
            case 't':
                threads = value;
                break;
            default:
                usage(argv[0]);
                break;
        }
    }

    if (sse2_supported())
        printf("Using SSE2 core.\n");
    else
        printf("Using original algorithm.\n");

    if (addr == 0)
        usage(argv[0]);

    // Autodetect threads
    if(threads == 0)
        threads = getNumberOfCores();

    // No nonces given: use whole disk
    if(nonces == 0) {
        unsigned long long fs = freespace("./");
        if(fs <= FREE_SPACE) {
            printf("Not enough free space on device\n");
            exit(-1);
        }
        fs -= FREE_SPACE;

        nonces = (unsigned long long)(fs / PLOT_SIZE);
    }

    // Autodetect stagger size
    if(staggersize == 0) {
        // use 80% of memory
        unsigned long long memstag = (freemem() * 0.8) / PLOT_SIZE;

        if(nonces < memstag) {
            // Small stack: all at once
            staggersize = nonces;
        } else {
            // Determine stagger that (almost) fits nonces
            for (int i = memstag; i >= 1000; i--) {
                if( (nonces % i) < 1000) {
                    staggersize = i;
                    nonces-= (nonces % i);
                    i = 0;
                }
            }
        }
    }

    // 32 Bit and above 4GB?
    if( sizeof( void* ) < 8 ) {
        if( staggersize > 15000 ) {
            printf("Cant use stagger sizes above 15000 with 32-bit version\n");
            exit(-1);
        }
    }

    // Adjust according to stagger size
    if(nonces % staggersize != 0) {
        nonces -= nonces % staggersize;
        nonces += staggersize;
        printf("Adjusting total nonces to %u to match stagger size\n", nonces);
    }

    printf("Creating plots for nonces %llu to %llu (%u GB) using %u MB memory and %u threads\n", startnonce, (startnonce + nonces), (unsigned int)(nonces / 4 / 953), (unsigned int)(staggersize / 4), threads);

    cache = calloc( PLOT_SIZE, staggersize );

    if(cache == NULL) {
        printf("Error allocating memory. Try lower stagger size.\n");
        exit(-1);
    }

    char name[100];
    sprintf(name, "%llu_%llu_%u_%u", addr, startnonce, nonces, staggersize);

    ofd = open(name, O_CREAT | O_LARGEFILE | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(ofd < 0) {
        printf("Error opening file %s\n", name);
        exit(0);
    }

    // Threads:
    noncesperthread = (unsigned long)(staggersize / threads);

    if(noncesperthread == 0) {
        threads = staggersize;
        noncesperthread = 1;
    }

    pthread_t worker[threads], writeworker;
    unsigned long long nonceoffset[threads];

    unsigned long long astarttime;
    wcache=cache;

    for(run = 0; run < nonces; run += staggersize) {
        astarttime = getMS();

        for (int i = 0; i < threads; i++) {
            nonceoffset[i] = startnonce + i * noncesperthread;

            if(pthread_create(&worker[i], NULL, work_i, &nonceoffset[i])) {
                printf("Error creating thread. Out of memory? Try lower stagger size / less threads\n");
                exit(-1);
            }
        }

        // Wait for Threads to finish;
        for(int i = 0; i < threads; i++) {
            pthread_join(worker[i], NULL);
        }

        // Run leftover nonces
        for(int i = threads * noncesperthread; i<staggersize; i++)
            nonce(addr, startnonce + i, (unsigned long long)i);

        // Write plot to disk:
        starttime=astarttime;
        lastrun=run+staggersize;
        if(pthread_create(&writeworker, NULL, writecache, (void *)NULL)) {
            printf("Error creating thread. Out of memory? Try lower stagger size / less threads\n");
            exit(-1);
        }
        pthread_join(writeworker, NULL);

        startnonce += staggersize;
    }

    close(ofd);

    printf("\nFinished plotting.\n");
    return 0;
}

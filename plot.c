#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include <sys/time.h>

#include "shabal.h"
#include "mshabal.h"
#include "helper.h"
#include "nonce.h"
#include "opts.h"

char *cache;

struct worker_args_t {
    uint64_t i;
    struct opts_t opts;
};

uint64_t get_ms();
void *work_i(void *x);
void print_stats(struct opts_t o, uint64_t start_ms, uint64_t nr, uint64_t dnr);

int main(int argc, char **argv) {
    struct opts_t o = get_opts(argc, argv);

    printf("Creating plot for nonces %llu to %llu (%u GB) using %u MB memory and %u threads\n",
            o.start_nonce,
            (o.start_nonce + o.num_nonces),
            (unsigned int)(o.num_nonces / 4 / 953),
            (unsigned int)(o.stagger_size / 4),
            o.num_threads);

    cache = calloc(PLOT_SIZE, o.stagger_size);

    if (cache == NULL) {
        printf("error: allocating cache memory (%llu MB). Try lower stagger size.\n",
               PLOT_SIZE*o.stagger_size/1024/1024);
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
    uint64_t nonceoffset[o.num_threads];

    for (uint64_t nr = 0; nr < o.num_nonces; nr += o.stagger_size) {
        uint64_t start_ms = get_ms();

        for (unsigned i = 0; i < o.num_threads; i++) {
            nonceoffset[i] = o.start_nonce + i * o.nonces_per_thread;

            struct worker_args_t *args = malloc(sizeof(struct worker_args_t));
            args->i = nonceoffset[i];
            args->opts = o;

            if (pthread_create(&worker[i], NULL, work_i, args)) {
                printf("error: unable to create thread. Out of memory? "
                       "Try lower stagger size / less threads\n");
                exit(-1);
            }
        }

        // wait for threads to finish
        for (unsigned i = 0; i < o.num_threads; i++) {
            pthread_join(worker[i], NULL);
            print_stats(o, start_ms, nr, i*o.nonces_per_thread);
        }

        // run leftover nonces
        for (uint64_t i = o.num_threads * o.nonces_per_thread; i < o.stagger_size; i++)
            nonce(o.addr, o.start_nonce + i, i, o.stagger_size);

        // write plot to disk
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
    uint64_t i = args->i;

    for (uint64_t n = 0; n < o.nonces_per_thread; n++) {
        if (o.use_sse2) {
            if (n + 4 < o.nonces_per_thread) {
                mnonce(o.addr,
                      (i + n + 0), (i + n + 1), (i + n + 2), (i + n + 3),
                      (i - o.start_nonce + n + 0),
                      (i - o.start_nonce + n + 1),
                      (i - o.start_nonce + n + 2),
                      (i - o.start_nonce + n + 3),
                      o.stagger_size);

                n += 3;
            } else {
               nonce(o.addr, (i + n), (i - o.start_nonce + n), o.stagger_size);
            }
        } else {
            nonce(o.addr, (i + n), (i - o.start_nonce + n), o.stagger_size);
        }
    }

    return NULL;
}

// dnr = "delta nr", how many nr's we are past our original base nr
// dnr = (thread_i) * o.nonces_per_thread);
void print_stats(struct opts_t o, uint64_t start_ms, uint64_t nr, uint64_t dnr) {
    // calculate percentage complete and nonces/min
    int percent = (int)(100 * (nr + dnr) / o.num_nonces);
    double minutes = (double) (get_ms() - start_ms) / (60 * 1000000);
    int speed = (int)(o.stagger_size / minutes);
    int m = (int)(o.num_nonces - nr) / speed;
    int h = (int)(m / 60);
    m -= h * 60;

    // print stats
    printf("\r%d%% | %d nonces/minute | %d:%02d left", percent, speed, h, m);
    fflush(stdout);
}

uint64_t get_ms() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return ((uint64_t)time.tv_sec * 1000000) + time.tv_usec;
}


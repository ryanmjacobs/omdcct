#ifndef OPTS_H
#define OPTS_H

#include <stdint.h>

struct opts_t {
    // required
    uint64_t addr;

    // optional, otherwise calculated
    int use_sse2;
    uint64_t num_threads;
    uint64_t num_nonces;
    uint64_t stagger_size;
    uint64_t start_nonce;

    // pure calculations
    uint64_t nonces_per_thread;
};

struct opts_t get_opts(int argc, char **argv);

#endif

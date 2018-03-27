#ifndef OPTS_H
#define OPTS_H

struct opts_t {
    // required
    unsigned long long addr;

    // optional, otherwise calculated
    int use_sse2;
    unsigned int num_threads;
    unsigned long long num_nonces;
    unsigned long long stagger_size;
    unsigned long long start_nonce;

    // pure calculations
    unsigned int noncesperthread;
};

struct opts_t get_opts(int argc, char **argv);

#endif

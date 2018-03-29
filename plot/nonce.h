#ifndef NONCE_H
#define NONCE_H

#include <stdint.h>

int mnonce(
    uint64_t addr,
    uint64_t nonce1,
    uint64_t nonce2,
    uint64_t nonce3,
    uint64_t nonce4,

    uint64_t cachepos1,
    uint64_t cachepos2,
    uint64_t cachepos3,
    uint64_t cachepos4,

    uint64_t stagger_size);

void nonce(uint64_t addr,
           uint64_t nr,
           uint64_t cachepos,
           uint64_t stagger_size);

#endif

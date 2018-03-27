#ifndef NONCE_H
#define NONCE_H

int mnonce(
    unsigned long long int addr,
    unsigned long long int nonce1,
    unsigned long long int nonce2,
    unsigned long long int nonce3,
    unsigned long long int nonce4,

    unsigned long long cachepos1,
    unsigned long long cachepos2,
    unsigned long long cachepos3,
    unsigned long long cachepos4,

    unsigned long long stagger_size);

void nonce(unsigned long long int addr,
           unsigned long long int nr,
           unsigned long long cachepos,
           unsigned long long stagger_size);

#endif

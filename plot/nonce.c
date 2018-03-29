#include <stdlib.h>
#include <string.h>

#include "nonce.h"
#include "shabal.h"
#include "mshabal.h"
#include "helper.h"
#include "opts.h"

extern char *cache;
extern struct opts_t *opts;

static void set_nonce(char *gendata, uint64_t *nonce) {
    for (int i = 0; i < 8; i++)
        gendata[PLOT_SIZE+8+i] = *((char *)nonce+7-i);
}

void nonce(uint64_t addr,
           uint64_t nr,
           uint64_t cachepos,
           uint64_t stagger_size)
{
    shabal_context x;
    char gendata[16 + PLOT_SIZE];

    // prepend gendata with addr and nr
    for (int i = 0; i < 8; i++) {
        gendata[PLOT_SIZE+i]   = *((char *)&addr+7-i);
        gendata[PLOT_SIZE+8+i] = *((char *)&nr  +7-i);
    }

    // generate each of our hashes, backwards from PLOT_SIZE (8192)
    for (int i = PLOT_SIZE; i > 0; i -= HASH_SIZE) {
        shabal_init(&x, 256);
        int len = PLOT_SIZE + 16 - i;
        if (len > HASH_CAP)
            len = HASH_CAP;
        shabal(&x, &gendata[i], len);
        shabal_close(&x, 0, 0, &gendata[i - HASH_SIZE]);
    }

    // create final hash
    char final[32];
    shabal_init(&x, 256);
    shabal(&x, gendata, 16 + PLOT_SIZE);
    shabal_close(&x, 0, 0, final);

    // XOR each hash with the final hash
    uint64_t *start = (uint64_t*)gendata;
    uint64_t *fhash = (uint64_t*)&final;
    for (int i = 0; i < PLOT_SIZE; i += 32) {
        *(start++) ^= fhash[0];
        *(start++) ^= fhash[1];
        *(start++) ^= fhash[2];
        *(start++) ^= fhash[3];
    }

    // Sort them:
    for (uint64_t i = 0; i < PLOT_SIZE; i += 64)
        memmove(&cache[cachepos*64 + i*stagger_size], &gendata[i], 64);
}

int mnonce(uint64_t addr,
    uint64_t nonce1,
    uint64_t nonce2,
    uint64_t nonce3,
    uint64_t nonce4,

    uint64_t cachepos1,
    uint64_t cachepos2,
    uint64_t cachepos3,
    uint64_t cachepos4,
    uint64_t stagger_size)
{
    char final1[32], final2[32], final3[32], final4[32];
    char gendata1[16 + PLOT_SIZE],
         gendata2[16 + PLOT_SIZE],
         gendata3[16 + PLOT_SIZE],
         gendata4[16 + PLOT_SIZE];

    // prepend gendata with addr
    for (int i = 0; i < 8; i++)
        gendata1[PLOT_SIZE+i] = *((char *)&addr+7-i);

    // clone gendata1 to gendata{2,3,4}
    for (int i = PLOT_SIZE; i <= PLOT_SIZE + 7; ++i) {
        gendata2[i] = gendata1[i];
        gendata3[i] = gendata1[i];
        gendata4[i] = gendata1[i];
    }

    set_nonce(gendata1, &nonce1);
    set_nonce(gendata2, &nonce2);
    set_nonce(gendata3, &nonce3);
    set_nonce(gendata4, &nonce4);

    mshabal_context x;

    for (int i = PLOT_SIZE; i > 0; i -= HASH_SIZE) {
        sse4_mshabal_init(&x, 256);

        int len = PLOT_SIZE + 16 - i;
        if (len > HASH_CAP)
            len = HASH_CAP;

        sse4_mshabal(&x, &gendata1[i], &gendata2[i],
                         &gendata3[i], &gendata4[i], len);
        sse4_mshabal_close(&x, 0, 0, 0, 0, 0,
                &gendata1[i - HASH_SIZE],
                &gendata2[i - HASH_SIZE],
                &gendata3[i - HASH_SIZE],
                &gendata4[i - HASH_SIZE]);
    }

    sse4_mshabal_init(&x, 256);
    sse4_mshabal(&x, gendata1, gendata2, gendata3, gendata4, 16 + PLOT_SIZE);
    sse4_mshabal_close(&x, 0, 0, 0, 0, 0, final1, final2, final3, final4);

    // XOR with final
    for (int i = 0; i < PLOT_SIZE; i++) {
        gendata1[i] ^= (final1[i % 32]);
        gendata2[i] ^= (final2[i % 32]);
        gendata3[i] ^= (final3[i % 32]);
        gendata4[i] ^= (final4[i % 32]);
    }

    // Sort them:
    for (uint64_t i = 0; i < PLOT_SIZE; i += 64) {
        memmove(&cache[cachepos1*64 + i*stagger_size], &gendata1[i], 64);
        memmove(&cache[cachepos2*64 + i*stagger_size], &gendata2[i], 64);
        memmove(&cache[cachepos3*64 + i*stagger_size], &gendata3[i], 64);
        memmove(&cache[cachepos4*64 + i*stagger_size], &gendata4[i], 64);
    }

    return 0;
}

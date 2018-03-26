#include <stdlib.h>
#include <string.h>

#include "nonce.h"
#include "shabal.h"
#include "mshabal.h"
#include "helper.h"

extern char *cache;
extern unsigned int staggersize;

#define PLOT_SIZE	(4096 * 64)
#define HASH_SIZE	32
#define HASH_CAP	4096

#define SET_NONCE(gendata, nonce)\
  xv = (char*)&nonce;\
  gendata[PLOT_SIZE + 8] = xv[7];\
  gendata[PLOT_SIZE + 9] = xv[6];\
  gendata[PLOT_SIZE + 10] = xv[5];\
  gendata[PLOT_SIZE + 11] = xv[4];\
  gendata[PLOT_SIZE + 12] = xv[3];\
  gendata[PLOT_SIZE + 13] = xv[2];\
  gendata[PLOT_SIZE + 14] = xv[1];\
  gendata[PLOT_SIZE + 15] = xv[0]

void nonce(unsigned long long int addr, unsigned long long int nr, unsigned long long cachepos) {
    char final[32];
    char gendata[16 + PLOT_SIZE];

    // prepend gendata with addr
    for (int i = 0; i < 8; i++)
        gendata[PLOT_SIZE+i] = *((char *)&addr+7-i);

    // prepend gendata with nr
    for (int i = 0; i < 8; i++)
        gendata[PLOT_SIZE+8+i] = *((char *)&nr+7-i);

    shabal_context x;

    // generate each of our hashes
    for (int i = PLOT_SIZE; i > 0; i -= HASH_SIZE) {
        shabal_init(&x, 256);
        int len = PLOT_SIZE + 16 - i;
        if (len > HASH_CAP)
            len = HASH_CAP;
        shabal(&x, &gendata[i], len);
        shabal_close(&x, 0, 0, &gendata[i - HASH_SIZE]);
    }

    // create final hash
    shabal_init(&x, 256);
    shabal(&x, gendata, 16 + PLOT_SIZE);
    shabal_close(&x, 0, 0, final);

    unsigned long long *start = (unsigned long long*)gendata;
    unsigned long long *fint = (unsigned long long*)&final;

    // XOR each hash with the final hash
    for (int i = 0; i < PLOT_SIZE; i += 32) {
        *start ^= fint[0]; start ++;
        *start ^= fint[1]; start ++;
        *start ^= fint[2]; start ++;
        *start ^= fint[3]; start ++;
    }

    // Sort them:
    for (int i = 0; i < PLOT_SIZE; i += 64)
        memmove(&cache[cachepos * 64 + (unsigned long long)i * staggersize], &gendata[i], 64);
}

int mnonce(unsigned long long int addr,
    unsigned long long int nonce1,
    unsigned long long int nonce2,
    unsigned long long int nonce3,
    unsigned long long int nonce4,

    unsigned long long cachepos1,
    unsigned long long cachepos2,
    unsigned long long cachepos3,
    unsigned long long cachepos4)
{
    char final1[32], final2[32], final3[32], final4[32];
    char gendata1[16 + PLOT_SIZE],
         gendata2[16 + PLOT_SIZE],
         gendata3[16 + PLOT_SIZE],
         gendata4[16 + PLOT_SIZE];

    char *xv = (char*)&addr;

    gendata1[PLOT_SIZE] = xv[7];
    gendata1[PLOT_SIZE + 1] = xv[6];
    gendata1[PLOT_SIZE + 2] = xv[5];
    gendata1[PLOT_SIZE + 3] = xv[4];
    gendata1[PLOT_SIZE + 4] = xv[3];
    gendata1[PLOT_SIZE + 5] = xv[2];
    gendata1[PLOT_SIZE + 6] = xv[1];
    gendata1[PLOT_SIZE + 7] = xv[0];

    for (int i = PLOT_SIZE; i <= PLOT_SIZE + 7; ++i) {
        gendata2[i] = gendata1[i];
        gendata3[i] = gendata1[i];
        gendata4[i] = gendata1[i];
    }

    SET_NONCE(gendata1, nonce1);
    SET_NONCE(gendata2, nonce2);
    SET_NONCE(gendata3, nonce3);
    SET_NONCE(gendata4, nonce4);

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
    for (int i = 0; i < PLOT_SIZE; i += 64) {
        memmove(&cache[cachepos1 * 64 + (unsigned long long)i * staggersize], &gendata1[i], 64);
        memmove(&cache[cachepos2 * 64 + (unsigned long long)i * staggersize], &gendata2[i], 64);
        memmove(&cache[cachepos3 * 64 + (unsigned long long)i * staggersize], &gendata3[i], 64);
        memmove(&cache[cachepos4 * 64 + (unsigned long long)i * staggersize], &gendata4[i], 64);
    }

    return 0;
}

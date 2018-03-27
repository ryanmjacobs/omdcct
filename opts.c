#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "opts.h"
#include "helper.h"

// Leave 5GB free space
#define FREE_SPACE	(unsigned long long)5 * 1000 * 1000 * 1000
#define PLOT_SIZE	(4096 * 64)

static void usage(const char *progname, int error) {
    fprintf(error ? stderr : stdout,
            "Usage: %s -k KEY [-s STARTNONCE] [-n NONCES] [-m STAGGERSIZE] "
            "[-t THREADS] [-x 0]\n", progname);
    exit(error);
}

struct opts_t get_opts(int argc, char **argv) {
    srand(time(NULL));
    struct opts_t o = {
        .use_sse2 = sse2_supported(),
        .num_threads = num_cores(),
        .start_nonce = (unsigned long long)rand() * (1 << 30) + rand()
    };

    if (argc < 2)
        usage(argv[0], 1);

    for (int i = 1; i < argc; i++) {
        // Ignore unknown argument
        if(argv[i][0] != '-')
            continue;

        // flag must be one character
        // flag must be followed by one value
        if (argv[i][2] != '\0' || i > argc-1)
            usage(argv[0], 1);

        char flag = argv[i][1];
        char *parse = argv[++i];

        // to-be-parsed value must exist
        if (parse == NULL)
            usage(argv[0], 1);

        unsigned long long value = strtoull(parse, 0, 10);

        switch (flag) {
            case 'k':
                o.addr = value;
                break;
            case 's':
                o.start_nonce = value;
                break;
            case 'n':
                o.num_nonces = value;
                break;
            case 'm':
                o.stagger_size = value;
                break;
            case 't':
                o.num_threads = value;
                break;
            case 'x':
                o.use_sse2 = sse2_supported() && value;
                break;
            case 'h':
            default:
                usage(argv[0], flag != 'h');
                break;
        }
    }

    if (o.addr == 0)
        usage(argv[0], 1);

    // determine amount of nonces
    if (o.num_nonces == 0) {
        unsigned long long fs = freespace("./");

        if(fs <= FREE_SPACE) {
            printf("Not enough free space on device\n");
            exit(-1);
        }
        fs -= FREE_SPACE;

        o.num_nonces = (unsigned long long)(fs / PLOT_SIZE);
    }

    // Autodetect stagger size
    if (o.stagger_size == 0) {
        // use 80% of memory
        unsigned long long memstag = (freemem() * 0.8) / PLOT_SIZE;

        if (o.num_nonces < memstag) {
            // Small stack: all at once
            o.stagger_size = o.num_nonces;
        } else {
            // Determine stagger that (almost) fits nonces
            for (int i = memstag; i >= 1000; i--) {
                if ((o.num_nonces % i) < 1000) {
                    o.stagger_size = i;
                    o.num_nonces -= (o.num_nonces % i);
                    i = 0;
                }
            }
        }
    }

    // calculate nonces per thread 
    o.noncesperthread = (unsigned long)(o.stagger_size / o.num_threads);
    if(o.noncesperthread == 0) {
        o.num_threads = o.stagger_size;
        o.noncesperthread = 1;
    }


    return o;
}

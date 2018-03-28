#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>

void p_ensure(int predicate, const char *msg) {
    if (!predicate) {
        perror(msg);
        exit(1);
    }
}

void ensure(int predicate, const char *msg) {
    if (!predicate) {
        fputs(msg, stderr);
        exit(1);
    }
}

struct plotfile_t { 
    const char *fname;
    uint64_t addr;    // burst address
    uint64_t snonce;  // start nonce
    uint64_t nonces;  // number of nonces
    uint64_t stagger; // stagger size
};

void print_plotfile(struct plotfile_t *pf) {
    printf("      fname: %s\n",    pf->fname);
    printf("       addr: %lu\n",   pf->addr);
    printf("start nonce: %lu\n",   pf->snonce);
    printf("num. nonces: %lu\n",   pf->nonces);
    printf("    stagger: %lu\n\n", pf->stagger);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <addr>\n", argv[0]);
        return 1;
    }

    // open the current directory
    DIR *dir;
    struct dirent *ent;
    p_ensure((dir = opendir(".")) != NULL, "opendir(\"./\")");

    // read in plotfiles that match our address
    char *addr = argv[1];
    while ((ent = readdir(dir))) {
        int fname_matches = !strncmp(addr, ent->d_name, strlen(addr));
        int readable = !access(ent->d_name, F_OK);

        if (fname_matches && readable) {
            struct plotfile_t *pf = malloc(sizeof(struct plotfile_t));
            pf->fname = ent->d_name;
            sscanf(pf->fname, "%lu_%lu_%lu_%lu",
                   &pf->addr, &pf->snonce, &pf->nonces, &pf->stagger);
            print_plotfile(pf);
        }
    }

    return 0;
}

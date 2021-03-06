
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <dirent.h>

// TODO: figure out what functions use perror()
// TODO: find better way of dealing with hard max of 64 plot files

#define MAX_PLOT_FILES 64

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

void upload(const char *fname) {
    FILE *fp = fopen(fname, "rb");
    p_ensure(fp != NULL, "fopen()");

    char cmd[1024];
    sprintf(cmd, "bash upload.sh %s", fname);
    system(cmd);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s <addr>\n", argv[0]);
        return 1;
    }

    // "globals"
    unsigned pf_cnt = 0;
    struct plotfile_t *plotfiles[MAX_PLOT_FILES];

    const char *addr = strcmp(argv[1], "default")
        ? argv[1] : "5801048965275211042";

    // open the current directory
    DIR *dir;
    struct dirent *ent;
    p_ensure((dir = opendir(".")) != NULL, "opendir(\"./\")");

    // read in plotfiles that match our address
    while ((ent = readdir(dir))) {
        if (pf_cnt > MAX_PLOT_FILES)
            break;

        int fname_matches = !strncmp(addr, ent->d_name, strlen(addr));
        int readable = !access(ent->d_name, F_OK);

        if (fname_matches && readable) {
            // parse plotfile filename
            struct plotfile_t *pf = malloc(sizeof(struct plotfile_t));
            pf->fname = strdup(ent->d_name);
            sscanf(pf->fname, "%lu_%lu_%lu_%lu",
                   &pf->addr, &pf->snonce, &pf->nonces, &pf->stagger);
            print_plotfile(pf);

            // push plotfile onto array
            plotfiles[pf_cnt++] = pf;
        }
    }
    closedir(dir);

    // exit if no plotfiles to process
    ensure(pf_cnt > 0, "error: no plotfiles located\n");

    // create google drive folder ID
    system("bash create_drive_folder.sh");

    // create and push each scoop
    for (unsigned scoop_idx = 0; scoop_idx < 4096; scoop_idx++) {
        // create filename
        char fname[256];
        sprintf(fname, "_%s_%u.scoops", addr, scoop_idx);

        // open file (and create if necessary)
        FILE *scoop_fp = fopen(fname, "ab");
        p_ensure(scoop_fp != NULL, "fopen()");

        // loop through plotfiles and dump their particular scoops
        for (unsigned pf_idx = 0; pf_idx < pf_cnt; pf_idx++) {
            // grab the i-th plotfile
            struct plotfile_t *pf = plotfiles[pf_idx];
            FILE *plot_fp = fopen(pf->fname, "rb");
            p_ensure(plot_fp != NULL, "fopen()");

            // push plot meta data
            fprintf(scoop_fp, "(%lu_%lu)", pf->snonce, pf->nonces);

            // push plot data
            for (uint64_t x = 0; x < pf->nonces; x++) {
                uint8_t scoop[64];
                long pos = (x*8192*32) + scoop_idx*64;

                fseek(plot_fp, pos, SEEK_SET);
                fread(&scoop,  64, 1, plot_fp);
                fwrite(&scoop, 64, 1, scoop_fp);
            }

            // close plotfile
            fclose(plot_fp);
        }

        fclose(scoop_fp);
        upload(fname);
        unlink(fname);
    }

    // free plotfiles
    for (unsigned i = 0; i < pf_cnt; i++) {
        free(plotfiles[i]);
    }

    return 0;
}

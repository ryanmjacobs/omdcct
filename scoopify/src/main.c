#include <stdio.h>
#include <string.h>
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

const char *addr = "123";
int main(void) {
    DIR *dir;
    struct dirent *ent;
    p_ensure((dir = opendir(".")) != NULL, "opendir(\"./\")");

    while ((ent = readdir(dir))) {
        if (!strncmp(addr, ent->d_name, strlen(addr)))
            puts(ent->d_name);
    }

    return 0;
}

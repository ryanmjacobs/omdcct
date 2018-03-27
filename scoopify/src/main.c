#include <stdio.h>
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

int main(void) {
    DIR *dir;
    struct dirent *ent;
    p_ensure((dir = opendir(".")) != NULL, "opendir(\"./\")");

    while ((ent = readdir(dir))) {
        printf("%s\n", ent->d_name);
    }

    return 0;
}

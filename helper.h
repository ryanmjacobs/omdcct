#ifndef HELPER_H
#define HELPER_H

int xstr2strr(char *buf, unsigned bufsize, const char *in);
int num_cores();
int hostname_to_ip(char * hostname , char* ip);
unsigned long long freespace(char *path);
unsigned long long freemem();
int sse2_supported();

#endif

#include "util.h"

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int write_fully(int fd, void * data, size_t size) {
    ssize_t result;
    ssize_t nwrote = 0;

    while (nwrote < (ssize_t)size) {
        result = write(fd, ((char*)data)+nwrote, size-nwrote);
        if (result == -1) {
            perror("writing fully");
            return -1;
        }
        nwrote += result;
    }
    return 0;
}

int TCP_socket(int *dst) {
    int sock;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1)
    {
        perror("creating socket");
        return -1;
    }

    *dst = sock;
    return 0;
}

int open_file(FILE **dst, const char * file_name, const char * mode) {
    FILE *file;

    file = fopen(file_name, mode);
    if (file == NULL) {
        perror("open file");
        return -1;
    }

    *dst = file;
    return 0;
}

int init_addr(struct sockaddr_in *dst, const char *ip, in_port_t port) {
    (*dst).sin_family = PF_INET;
    (*dst).sin_port = htons(port);
    (*dst).sin_addr.s_addr = inet_addr(ip);
    if((*dst).sin_addr.s_addr ==  (in_addr_t)-1)
    {
        perror("constructing address"); // TODO: does address failure actually set errno?
        return -1;
    }

    return 0;
}

int init_addr_any(struct sockaddr_in *dst, in_port_t port) {
    (*dst).sin_family = PF_INET;
    (*dst).sin_port = htons(port);
    (*dst).sin_addr.s_addr = INADDR_ANY;
    if((*dst).sin_addr.s_addr ==  (in_addr_t)-1)
    {
        perror("constructing address"); // TODO: does address failure actually set errno?
        return -1;
    }

    return 0;
}

int parse_port(in_port_t *dst, const char *buff, int radix)
{
    char *end;
    long sl;
    in_port_t port;
    const char *msg;

    errno = 0;
    sl = strtol(buff, &end, radix);

    if(end == buff)
    {
        msg = "not a decimal number";
    }
    else if(*end != '\0')
    {
        msg = "extra characters at end of input";
    }
    else if((LONG_MIN == sl || LONG_MAX == sl) && ERANGE == errno)
    {
        msg = "out of range of type long";
    }
    else if(sl > UINT16_MAX)
    {
        msg = "greater than UINT16_MAX";
    }
    else if(sl < 0)
    {
        msg = "less than 0";
    }
    else
    {
        msg = NULL;
    }

    if(msg)
    {
        (void) fprintf(stderr, "parsing port: %s\n", msg);
        return -1;
    }

    *dst = (in_port_t)sl;

    return 0;
}

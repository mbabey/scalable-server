#include "util.h"

#include <arpa/inet.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

int init_connection(int sock_fd, struct sockaddr_in *addr) {
    int result;

    result = connect(sock_fd, (struct sockaddr *)addr, sizeof(struct sockaddr_in));
    if(result == -1) {
        perror("connect");
        return -1;
    }

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

int set_sock_blocking(int fd, bool blocking) {
    int result;
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("getting socket flags");
        return -1;
    }
    flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);

    result = fcntl(fd, F_SETFL, flags);
    if (result == -1) {
        perror("setting socket flags");
        return -1;
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

int init_addr(struct sockaddr_in *dst, const char *ip, in_port_t port) {
    (*dst).sin_family = AF_INET;
    (*dst).sin_port = htons(port);
    (*dst).sin_addr.s_addr = inet_addr(ip);
    if((*dst).sin_addr.s_addr ==  (in_addr_t)-1)
    {
        perror("constructing address");
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
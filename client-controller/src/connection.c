#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "connection.h"

#define START 1
#define STOP 2

int send_start(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    ssize_t nwrote;

    int result = 0;
    for (int i = 0; i < s->num_conns; i++)
    {
        uint16_t net_start = htons(START);
        while ((nwrote = write(s->accepted_fds[i], &net_start, sizeof(net_start))) == 0);
        if (nwrote == -1)
        {
            perror("writing start to clients");
            result = -1;
        }
    }
    s->started = true;
    return result;
}

int send_stop(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    ssize_t nwrote;

    int result = 0;
    for (int i = 0; i < s->num_conns; i++)
    {
        uint16_t net_stop = htons(STOP);
        while ((nwrote = write(s->accepted_fds[i], &net_stop, sizeof(net_stop))) == 0);
        if (nwrote == -1)
        {
            perror("writing stop to clients");
            result = -1;
        }
    }
    s->started = false;
    return result;
}

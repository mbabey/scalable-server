#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "connection.h"

static const uint16_t start = 1;
static const uint16_t stop = 0;

int send_start(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    ssize_t nwrote;

    int result = 0;
    for (int i = 0; i < s->num_conns; i++)
    {
        uint16_t net_start = htons(start);
        while ((nwrote = write(s->accepted_fds[i], &net_start, sizeof(start))) == 0);
        if (nwrote == -1)
        {
            (void) fprintf(stderr, "%s\n", strerror(errno));
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
        uint16_t net_stop = htons(stop);
        while ((nwrote = write(s->accepted_fds[i], &net_stop, sizeof(stop))) == 0);
        if (nwrote == -1)
        {
            (void) fprintf(stderr, "%s\n", strerror(errno));
            result = -1;
        }
    }
    s->started = false;
    return result;
}

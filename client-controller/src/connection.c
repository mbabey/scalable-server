#include "connection.h"

#include <util.h>

#include <unistd.h>
#include <string.h>

static uint16_t start = 1;
static uint16_t stop = 2;

int send_start(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);

    int result = 0;
    for (int i = 0; i < s->num_conns; i++)
    {
        uint16_t net_start = htons(start);
        result = write_fully(s->accepted_fds[i], &net_start, sizeof(net_start));
    }
    s->started = true;
    return result;
}

int send_stop(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);

    int result = 0;
    for (int i = 0; i < s->num_conns; i++)
    {
        uint16_t net_stop = htons(stop);
        result = write_fully(s->accepted_fds[i], &net_stop, sizeof(net_stop));
    }
    s->started = false;
    return result;
}

int send_data(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);

    int result = 0;
    uint16_t net_port = htons(s->server_port);
    uint32_t ip_size = (uint32_t) (strlen(s->server_ip) + 1);
    uint32_t net_ip_size = (htonl(ip_size));
    uint32_t net_data_size = (htonl((uint32_t)s->data_size));
    for (int i = 0; i < s->num_conns; i++)
    {
        if (write_fully(s->accepted_fds[i], &net_port, sizeof(net_port)) == -1) {
            result = -1;
        }
        if (write_fully(s->accepted_fds[i], &net_ip_size, sizeof(net_ip_size)) == -1) {
            result = -1;
        }
        if (write_fully(s->accepted_fds[i], s->server_ip, (size_t)ip_size) == -1) {
            result = -1;
        }
        if (write_fully(s->accepted_fds[i], &net_data_size, sizeof(net_data_size)) == -1) {
            result = -1;
        }
        if (write_fully(s->accepted_fds[i], s->data, s->data_size) == -1) {
            result = -1;
        }
    }

    return result;
}

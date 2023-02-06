#include "../include/state.h"

#include <util.h>

#include <string.h>
#include <dc_application/application.h>
#include <stdio.h>
#include <unistd.h>

#define BACKLOG 10

/**
 * start_listen
 * <p>
 * creates, binds, and listens on a TCP socket.
 * </p>
 * @param s the program state.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_evn struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
static int start_listen(struct state * s, struct dc_error * err, struct dc_env * env);

int init_state(const char *listen_port, struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);

    memset(s, 0, sizeof(struct state));

    s->listen_port = parse_port(listen_port, 10);
    if (s->listen_port == (in_port_t)0)
    {
        return -1;
    }

    return start_listen(s, err, env);
}

static int start_listen(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    int option;

    if(TCP_socket(&s->listen_fd) == -1)
    {
        return -1;
    }

    if (init_addr(&s->listen_addr, s->listen_port) == -1)
    {
        return -1;
    }

    option = 1;
    setsockopt(s->listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if(bind(s->listen_fd, (struct sockaddr *)&s->listen_addr, sizeof(struct sockaddr_in)) == -1)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }

    if(listen(s->listen_fd, BACKLOG) == -1)
    {
        fprintf(stderr, "%s\n", strerror(errno));
        return -1;
    }
    return 0;
}

int destroy_state(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    int result = 0;

    if (s->listen_fd)
    {
        result = close(s->listen_fd) == -1;
        if (result == -1)
        {
            fprintf(stderr, "%s\n", strerror(errno));
        }
    }

    for (int i = 0; i < s->num_conns; i++) {
        result = close(s->accepted_fds[i]) == -1;
        if (result == -1)
        {
            fprintf(stderr, "%s\n", strerror(errno));
        }
    }

    return result;
}

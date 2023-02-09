#include "run.h"

#include <thread.h>

#include <poll.h>
#include <unistd.h>

#define START 1
#define STOP 2

enum states {ERROR = -1, SUCCESS = 0, END = 1};

static int handle_controller(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env);

int run_state(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    bool exit;
    enum states result;
    struct pollfd fds[2];
    int timeout_msecs;

    timeout_msecs = 500;
    fds[0].fd = s->controller_fd;
    fds[0].events = POLLIN;

    exit = false;
    while(!exit)
    {
        result = poll(fds, 2, timeout_msecs);
        if (result == ERROR && errno != EINTR) // poll error (ignore interrupt error)
        {
            perror("polling controller socket");
            exit = true;
        }
        if (fds[0].revents && POLLIN) // listen_fd readable
        {
            result = handle_controller(&fds[0], s, err, env);
            if (result == END)
            {
                result = SUCCESS;
                exit = true;
            } else if (result == ERROR)
            {
                exit = true;
            }
        }
    }

    return result;
}

static int handle_controller(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    pfd->revents = 0;

    ssize_t nread;
    uint16_t command;

    while((nread = read(s->controller_fd, &command, sizeof(command))) == 0);
    if (nread == -1) {
        perror("reading controller command");
        return -1;
    }

    command = htons(command);
    switch(command) {
        case START:
            start_threads(s, err, env);
            return SUCCESS;
        case STOP:
            stop_threads(s, err, env);
            return END;
        default:
            (void) fprintf(stderr, "unknown command %d received from controller\n", command);
            return ERROR;
    }
}

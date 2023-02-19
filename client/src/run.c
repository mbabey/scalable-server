#include "run.h"

#include <thread.h>

#include <poll.h>
#include <unistd.h>

#define START 1
#define STOP 2
#define POLL_TIMEOUT_MSECS 500

enum states {ERROR = -1, SUCCESS = 0, END = 1};

/**
 * handle_controller
 * <p>
 * read a command from the controller and perform the appropriate action.
 * </p>
 * @param pfd pollfd to reset revents on.
 * @param s pointer to the state object.
 * @param err pointer to the dc_error struct.
 * @param env pointer to the dc_env struct.
 * @return 1 if STOP is received, 0 if START is received. -1 and set errno on failure.
 */
static int handle_controller(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env);

int run_state(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    bool exit;
    enum states result;
    struct pollfd fds[1];

    fds[0].fd = s->controller_fd;
    fds[0].events = POLLIN;

    exit = false;
    while(!exit)
    {
        result = poll(fds, 2, POLL_TIMEOUT_MSECS);
        if (result == ERROR && errno != EINTR) // poll error (ignore interrupt error)
        {
            perror("polling controller socket");
            exit = true;
        }
        if (fds[0].revents && POLLIN) // controller_fd readable
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

    ssize_t nread = 0;
    ssize_t result;
    uint16_t command;

    while(nread < (ssize_t)sizeof(command)) {
        result = read(s->controller_fd, &command, sizeof(command));
        if (result == -1) {
            perror("reading controller command");
            return -1;
        }
        nread += result;
    }

    command = ntohs(command);
    switch(command) {
        case START:
            if (start_threads(s, err, env) == -1) {
                return ERROR;
            }
            return SUCCESS;
        case STOP:
            if (stop_threads(err, env) == -1) {
                return ERROR;
            }
            return END;
        default:
            (void) fprintf(stderr, "unknown command %d received from controller\n", command);
            return ERROR;
    }
}

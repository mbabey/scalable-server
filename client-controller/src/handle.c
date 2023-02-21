#include "handle.h"

#include <connection.h>

#include <poll.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <arpa/inet.h>

#define START_COMMAND "start"

/**
 * handle_accept
 * <p>
 * handle a new connection by accepting and appending to state.accepted_fds.
 * </p>
 * @param pfd poll file descriptor to reset revents on.
 * @param s the program state struct.
 * @param err pointer to a dc_err struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
static int handle_accept(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * handle_stdin
 * <p>
 * check if a user input equals the start command. If so, send the start command to clients and then send server port,
 * server ip, and data.
 * </p>
 * @param pfd poll file descriptor to reset revents on.
 * @param s the program state struct.
 * @param err pointer to a dc_err struct.
 * @param env pointer to a dc_env struct.
 * @return 0 on wrong command, 1 on start. On failure, -1 and set errno.
 */
static int handle_stdin(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * wait_duration
 * <p>
 * print test start and display progress while waiting a duration.
 * </p>
 * @param s the program state struct.
 * @param err pointer to a dc_err struct.
 * @param env pointer to a dc_env struct.
 */
static void wait_duration(struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * set_signal_handling
 * <p>
 * sets signal handler for SIGINT.
 * </p>
 * @param sa pointer to a sigaction struct.
 */
static void set_signal_handling(struct sigaction *sa);

/**
 * signal_handler
 * <p>
 * sets sig_quit to 1 when a signal is received.
 * </p>
 * @param sig signal received.
 */
static void signal_handler(int sig);

static volatile sig_atomic_t sig_quit; // SIGINT flag
enum states {ERROR = -1, SUCCESS = 0, STARTED = 1};

int handle(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);

    bool exit;
    enum states result;
    struct pollfd fds[2];
    int timeout_msecs;
    struct sigaction sa;

    timeout_msecs = 500;
    fds[0].fd = s->listen_fd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    set_signal_handling(&sa);
    exit = false;
    sig_quit = 0;
    while(!exit && !sig_quit)
    {
        result = poll(fds, 2, timeout_msecs);
        if (result == ERROR && errno != EINTR) // poll error (ignore interrupt error)
        {
            perror("polling listening socket and stdin");
            exit = true;
        }
        if (fds[0].revents && POLLIN) // listen_fd readable
        {
            result = handle_accept(&fds[0], s, err, env);
            exit = result == ERROR;
        }
        if (fds[1].revents && POLLIN) // stdin readable
        {
            result = handle_stdin(&fds[1], s, err, env);
            if (result == STARTED)
            {
                result = SUCCESS;
                exit = true;
            } else if (result == ERROR) {
                exit = true;
            }
        }
    }

    return result;
}

static int handle_accept(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    pfd->revents = 0;

    struct sockaddr_in accept_addr;
    int fd;
    socklen_t accept_addr_len;

    accept_addr_len = sizeof(accept_addr);
    fd = accept(s->listen_fd, (struct sockaddr *)&accept_addr, &accept_addr_len);
    if (fd == -1)
    {
        perror("accepting client connection");
        return ERROR;
    }

    s->accepted_fds[s->num_conns] = fd;
    if (s->num_conns >= MAX_CONNS)
    {
        (void) fprintf(stderr, "Maximum number of connections reached (%d)", MAX_CONNS);
        return ERROR;
    }
    s->num_conns++;

    (void) fprintf(stdout, "%s connected\n",  inet_ntoa(accept_addr.sin_addr));
    return SUCCESS;
}

static int handle_stdin(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env)
{
    DC_TRACE(env);
    pfd->revents = 0;

    char buff[LINE_MAX];
    ssize_t nread;

    nread = read(STDIN_FILENO, &buff, LINE_MAX);
    if (nread == -1)
    {
        perror("reading stdin");
        return ERROR;
    }

    buff[strcspn(buff, "\n\r")] = 0; // trim trailing \n or \r from input

    if (strcmp(buff, START_COMMAND) == 0)
    {
        if (send_start(s, err, env) == -1) {
            return ERROR;
        }
        if (send_data(s, err, env)) {
            return ERROR;
        }
        wait_duration(s, err, env);
        return STARTED;
    }

    return SUCCESS;
}

static void wait_duration(struct state * s, struct dc_error * err, struct dc_env * env) {
    (void) fprintf(stdout, "Starting %d second load test with %d clients", s->wait_period_sec, s->num_conns);
    for (int i = 0; i < s->wait_period_sec && !sig_quit; i++) {
        (void) fprintf(stdout, ".");
        fflush(stdout);
        sleep(1);
    }
    (void) fprintf(stdout, "done\n");
}

static void set_signal_handling(struct sigaction *sa)
{
    int result;

    sigemptyset(&sa->sa_mask);
    sa->sa_flags = 0;
    sa->sa_handler = signal_handler;
    result = sigaction(SIGINT, sa, NULL);

    if(result == -1)
    {
        perror("sigaction");
    }
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
static void signal_handler(int sig)
{
    sig_quit = 1;
}
#pragma GCC diagnostic pop

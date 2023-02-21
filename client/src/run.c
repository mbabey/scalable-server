#include "run.h"

#include <thread.h>
#include <util.h>

#include <poll.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>

#define START 1
#define STOP 2
#define POLL_TIMEOUT_MSECS 500

enum states {ERROR = -1, SUCCESS = 0, END = 1};

/**
 * run_controller
 * <p>
 * runs the client with a controller that signals when to start and stop. Controller also provides server IP,
 * server port, and data.
 * </p>
 * @param s pointer to the state structure.
 * @param err pointer to the dc_error struct.
 * @param env pointer to to the dc_env struct.
 * @return 0 on success. -1 and set errno on failure.
 */
static int run_controller(struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * run_standalone
 * <p>
 * runs the client in standalone mode with the provided server IP, server port, data, and test duration.
 * </p>
 * @param s pointer to the state structure.
 * @param err pointer to the dc_error struct.
 * @param env pointer to to the dc_env struct.
 * @return 0 on success. -1 and set errno on failure.
 */
static int run_standalone(struct state * s, struct dc_error * err, struct dc_env * env);

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

/**
 * read_data
 * <p>
 * reads server port, server IP, and data from the controller in that order.
 * </p>
 * @param s pointer to the state structure.
 * @param err pointer to the dc_error structure.
 * @param env pointer to the dc_env structure.
 * @return 0 on success. -1 and set errno on failure.
 */
int read_data(struct state * s, struct dc_error * err, struct dc_env * env);

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

static volatile sig_atomic_t sig_quit; // SIGINT flag (used for standalone mode)

int run_state(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    if (s->standalone) {
        return run_standalone(s, err, env);
    }
    return run_controller(s, err, env);
}

static int run_controller(struct state * s, struct dc_error * err, struct dc_env * env) {
    bool exit;
    enum states result;
    struct pollfd fds[1];

    fds[0].fd = s->controller_fd;
    fds[0].events = POLLIN;

    exit = false;
    while(!exit)
    {
        result = poll(fds, 1, POLL_TIMEOUT_MSECS);
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

static int run_standalone(struct state * s, struct dc_error * err, struct dc_env * env) {
    struct sigaction sa;
    set_signal_handling(&sa);
    if (start_threads(s, err, env) == -1) return ERROR;
    wait_duration(s, err, env);
    return SUCCESS;
}

static int handle_controller(struct pollfd *pfd, struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    pfd->revents = 0;

    uint16_t command;
    if (read_fully(s->controller_fd, &command, sizeof(command)) == -1) return ERROR;
    command = ntohs(command);

    switch(command) {
        case START:
            if (read_data(s, err, env) == -1) {
                return ERROR;
            }
            if (start_threads(s, err, env) == -1) {
                return ERROR;
            }
            return SUCCESS;
        case STOP:
            return END;
        default:
            (void) fprintf(stderr, "unknown command %d received from controller\n", command);
            return ERROR;
    }
}

int read_data(struct state * s, struct dc_error * err, struct dc_env * env) {
    uint16_t net_port;
    uint32_t server_ip_size;
    uint32_t data_size;

    if (read_fully(s->controller_fd, &net_port, sizeof(net_port)) == -1) return ERROR;
    s->server_port = ntohs(net_port);

    if (read_fully(s->controller_fd, &server_ip_size, sizeof(server_ip_size)) == -1) return ERROR;
    server_ip_size = ntohl(server_ip_size);

    s->server_ip = malloc(server_ip_size + 1);
    if (s->server_ip == NULL) return ERROR;
    if (read_fully(s->controller_fd, s->server_ip, server_ip_size) == -1) return ERROR;
    s->server_ip[server_ip_size] = '\0';

    if (read_fully(s->controller_fd, &data_size, sizeof(data_size)) == -1) return ERROR;
    s->data_size = (off_t)ntohl(data_size);

    s->data = malloc(data_size);
    if (s->data == NULL) return ERROR;
    if (read_fully(s->controller_fd, s->data, s->data_size) == -1) return ERROR;

    return SUCCESS;
}

static void wait_duration(struct state * s, struct dc_error * err, struct dc_env * env) {
    (void) fprintf(stdout, "Starting %d second load test with 1 client", s->wait_period_sec);
    for (int i = 0; i < s->wait_period_sec && !sig_quit; i++) {
        (void) fprintf(stdout, ".");
        (void)fflush(stdout);
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
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

#include "../include/objects.h"
#include "../include/process_server.h"

#include <arpa/inet.h>
#include <dc_env/env.h>
#include <errno.h>
#include <mem_manager/manager.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h> // back compatability
#include <sys/types.h>  // back compatability
#include <time.h>
#include <unistd.h>

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables): must be non-const
/**
 * Whether the loop at the heart of the program should be running.
 */
volatile int GOGO_PROCESS = 1;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

/**
 * setup_signal_handler
 * @param sa sigaction struct to fill
 * @return 0 on success, -1 and set errno on failure
 */
static int setup_signal_handler(struct sigaction *sa, int signal);

/**
 * end_gogo_handler
 * <p>
 * Handler for SIGINT. Set the running loop conditional to 0.
 * </p>
 * @param signal the signal received
 */
static void end_gogo_handler(int signal);

/**
 * close_fd_report_undefined_error
 * <p>
 * Close a file descriptor and report an error which would make the file descriptor undefined.
 * </p>
 * @param fd the fd to close
 * @param err_msg the error message to print
 */
static void close_fd_report_undefined_error(int fd, const char *err_msg);

struct state_object *setup_process_state(struct memory_manager *mm)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof(struct state_object), mm);
    if (!so) // Depending on whether more is added to this state object, this if clause may go.
    {
        return NULL;
    }
    
    return so;
}

int open_process_server_for_listen(struct core_object *co, struct parent_struct *ps, struct sockaddr_in *listen_addr)
{
    DC_TRACE(co->env);
    int fd;
    
    fd = socket(PF_INET, SOCK_STREAM, 0); // NOLINT(android-cloexec-socket): SOCK_CLOEXEC dne
    if (fd == -1)
    {
        return -1;
    }
    
    if (bind(fd, (struct sockaddr *) listen_addr, sizeof(struct sockaddr_in)) == -1)
    {
        (void) close(fd);
        return -1;
    }
    
    if (listen(fd, CONNECTION_QUEUE) == -1)
    {
        (void) close(fd);
        return -1;
    }
    
    /* Only assign if absolute success. listen_fd == 0 can be used during teardown
     * to determine whether there is a socket to close. */
    ps->listen_fd = fd;
    
    return 0;
}

int run_process_server(struct core_object *co)
{
    
    
    return 0;
}

static int setup_signal_handler(struct sigaction *sa, int signal)
{
    sigemptyset(&sa->sa_mask);
    sa->sa_flags   = 0;
    sa->sa_handler = end_gogo_handler;
    if (sigaction(signal, sa, 0) == -1)
    {
        return -1;
    }
    return 0;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

static void end_gogo_handler(int signal)
{
    GOGO_PROCESS = 0;
}

#pragma GCC diagnostic pop

void destroy_process_state(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    
    // TODO: here is where we must tie up all of the processes. perhaps use signals?
    
    close_fd_report_undefined_error(so->listen_fd, "state of listen socket is undefined.");
    
    for (size_t sfd_num = 0; sfd_num < MAX_CONNECTIONS; ++sfd_num)
    {
        close_fd_report_undefined_error(*(so->client_fd + sfd_num), "state of client socket is undefined.");
    }
}

static void close_fd_report_undefined_error(int fd, const char *err_msg)
{
    if (close(fd) == -1)
    {
        switch (errno)
        {
            case EBADF: // Not a problem.
            {
                errno = 0;
                break;
            }
            default:
            {
                // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
                (void) fprintf(stderr, "Error: %s; %s\n", strerror(errno), err_msg);
            }
        }
    }
}

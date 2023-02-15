#include "../include/objects.h"
#include "../include/process_server.h"
#include "../include/setup.h"

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

int setup_process_server(struct core_object *co, struct state_object *so)
{
    so = setup_process_state(co->mm); // fixme: Will this allocate to the address in co? I think so, but not sure.
    if (!so)
    {
        return -1;
    }
    
    if (open_domain_socket_setup_semaphores(co, so) == -1)
    {
        return -1;
    }
    
    pid_t pid;
    
    // TODO: for each index in so->child_pids (i.e. up to NUM_CHILD_PROCESSES)
    pid = fork();
    if (pid > 0) // Parent
    {
        // TODO: Save to array at where pid is 0 (it is 0 because of calloc)
        
        so->parent = (struct parent_struct *) Mmm_calloc(1, sizeof(struct parent_struct), co->mm);
        if (!so->parent)
        {
            // TODO: do something complicated
        }
        so->child = NULL; // Here for clarity; will already be null.
    
        p_open_process_server_for_listen(co, so->parent, &so->listen_addr); // Listen on parent
    }
    else if (pid == 0) // Child
    {
        so->parent = NULL; // Here for clarity; will already be null.
        so->child = (struct child_struct *) Mmm_calloc(1, sizeof(struct child_struct), co->mm);
        if (!so->child)
        {
            // TODO: do something complicated
        }
    }
    
    return 0;
}

int p_run_process_server(struct core_object *co)
{
    // TODO: in this function the server process (parent) will poll active sockets then send them to the children.
    
    return 0;
}

int c_run_process_server(struct core_object *co)
{
    // TODO: in this function the child process will look for action on the domain socket, read a socket, then send the parent fd through the pipe when reading is done.
    
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
    
    close_fd_report_undefined_error(so->parent->listen_fd, "state of listen socket is undefined.");
    
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

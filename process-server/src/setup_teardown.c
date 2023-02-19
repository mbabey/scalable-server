#include "../include/setup_teardown.h"

#include <dc_env/env.h>
#include <mem_manager/manager.h>
#include <signal.h>
#include <string.h>
#include <sys/semaphore.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * open_semaphores
 * <p>
 * Open the read, write, and log semaphores. If an error occurs opening any one of them, close them all.
 * </p>
 * @param read_sem read/write pipe semaphores
 * @param log_sem log file semaphore
 * @return 0 on success, set errno and -1 on failure
 */
static int open_semaphores(sem_t *read_sem, sem_t *write_sem, sem_t *log_sem);

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

int open_pipe_semaphores_domain_sockets(struct core_object *co, struct state_object *so)
{
    DC_TRACE(co->env);
    
    if (pipe(so->c_to_p_pipe_fds) == -1) // Open pipe.
    {
        return -1;
    }
    
    if (open_semaphores(so->c_to_f_pipe_sems[READ], so->c_to_f_pipe_sems[WRITE], so->log_sem) == -1)
    {
        return -1;
    }
    
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, so->domain_fds) == -1) // lol I love linux
    {
        return -1;
    }
    
    return 0;
}

static int open_semaphores(sem_t *read_sem, sem_t *write_sem, sem_t *log_sem)
{
    read_sem  = sem_open(READ_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    write_sem = sem_open(WRITE_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    log_sem   = sem_open(LOG_SEM_NAME, O_CREAT, S_IRUSR | S_IWUSR, 0);
    if (read_sem == SEM_FAILED || write_sem == SEM_FAILED || log_sem == SEM_FAILED)
    {
        int err_save;
        err_save = errno;
        // Closing an unopened semaphore will return -1 and set errno = EINVAL, which can be ignored.
        sem_close(read_sem);
        sem_close(write_sem);
        sem_close(log_sem);
        // Unlinking an unopened semaphore will return -1 and set errno = ENOENT, which can be ignored.
        sem_unlink(READ_SEM_NAME);
        sem_unlink(WRITE_SEM_NAME);
        sem_unlink(LOG_SEM_NAME);
        errno = err_save;
        return -1;
    }
    
    return 0;
}

int
p_open_process_server_for_listen(struct core_object *co, struct parent_struct *parent, struct sockaddr_in *listen_addr)
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
    
    parent->listen_fd = fd;
    
    return 0;
}

void p_destroy_parent_state(struct core_object *co, struct state_object *so, struct parent_struct *parent)
{
    int status;
    
    FOR_EACH_CHILD_c // Send signals to child processes real quick.
    {
        kill(so->child_pids[c], SIGINT);
    }
    FOR_EACH_CHILD_c // Wait for child processes to wrap up.
    {
        waitpid(so->child_pids[c], &status, 0);
    }
    
    close_fd_report_undefined_error(so->c_to_p_pipe_fds[READ], "state of pipe read is undefined.");
    close_fd_report_undefined_error(so->domain_fds[WRITE], "state of parent domain socket is undefined.");
    close_fd_report_undefined_error(parent->listen_fd, "state of listen socket is undefined.");
    
    for (size_t sfd_num = 0; sfd_num < MAX_CONNECTIONS; ++sfd_num)
    {
        close_fd_report_undefined_error(*(parent->client_fds + sfd_num), "state of client socket is undefined.");
    }
    
    co->mm->mm_free(co->mm, parent);
    
    sem_close(so->c_to_f_pipe_sems[READ]);
    sem_close(so->c_to_f_pipe_sems[WRITE]);
    sem_close(so->log_sem);
    sem_unlink(READ_SEM_NAME);
    sem_unlink(WRITE_SEM_NAME);
    sem_unlink(LOG_SEM_NAME);
}

void c_destroy_child_state(struct core_object *co, struct state_object *so, struct child_struct *child)
{
    close_fd_report_undefined_error(so->c_to_p_pipe_fds[WRITE], "state of pipe write is undefined.");
    close_fd_report_undefined_error(so->domain_fds[READ], "state of child domain socket is undefined.");
    
    co->mm->mm_free(co->mm, child);
}

void close_fd_report_undefined_error(int fd, const char *err_msg)
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

#include "../include/objects.h"
#include "../include/one_to_one.h"

#include <errno.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <sys/socket.h> // back compatability
#include <sys/types.h>  // back compatability
#include <unistd.h>

/**
 * log
 * <p>
 * Log the information from one received message into the log file in Comma Separated Value file format.
 * </p>
 * @param co teh core object
 * @param so the state object
 * @param fd_num the file descriptor that was read
 * @param bytes the number of bytes read
 * @param start_time the start time of the read
 * @param end_time the end time of the read
 * @param elapsed_time_granular the elapsed time in seconds
 */
static void log(struct core_object *co, struct state_object *so, ssize_t bytes,
                time_t start_time, time_t end_time, double elapsed_time_granular);

struct state_object *setup_state(struct memory_manager *mm)
{
    struct state_object *so;
    
    so = (struct state_object *) Mmm_calloc(1, sizeof (struct state_object), mm);
    if (!so) // Depending on whether more is added to this state object, this if clause may go.
    {
        return NULL;
    }
    
    return so;
}

int open_server_for_listen(struct state_object *so, struct sockaddr_in *listen_addr)
{
    int fd;
    
    fd = socket(PF_INET, SOCK_STREAM, 0);
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
    so->listen_fd = fd;
    
    return 0;
}

int accept_conn(int listen_fd){
    struct sockaddr addr;
    socklen_t len = sizeof(addr);

    int fd = accept(listen_fd, &addr, &len);

    return fd;
}

enum msg_result {
    MSG_RESULT_SUCCESS,
    MSG_RESULT_ERROR,
    MSG_RESULT_CLOSED,
};

static int receive_message (struct core_object *co){
    uint32_t msg_size;
    ssize_t read_bytes = recv(co->so->client_fd, &msg_size, sizeof (msg_size), MSG_WAITALL); // TODO check return value
    if (read_bytes == 0) {
        return MSG_RESULT_CLOSED;
    } else if (read_bytes == -1) {
        return MSG_RESULT_ERROR;
    }
    msg_size = ntohl(msg_size);
    char buf[1024 * 1024];
    time_t start_time = time(NULL);
    clock_t start_time_granular = clock();

    // Reducing the size of the msg to reach the end of the msg.
    for (uint32_t remaining_bytes = msg_size; remaining_bytes > 0; remaining_bytes -= read_bytes) {
        read_bytes = recv(co->so->client_fd, &buf, sizeof(buf) < remaining_bytes ? sizeof(buf) : remaining_bytes, 0);
        if (read_bytes == 0) {
            return MSG_RESULT_CLOSED;
        } else if (read_bytes == -1) {
            return MSG_RESULT_ERROR;
        }
    }

    time_t  end_time = clock();
    clock_t end_time_granular = time(NULL);
    double elapsed_time_granular = (double) (end_time_granular - start_time_granular) / CLOCKS_PER_SEC;
    log(co, co->so, msg_size, start_time, end_time, elapsed_time_granular);

    ssize_t to_send = sizeof(msg_size);
    msg_size = htonl(msg_size);

    for (const char* size_p = (const char*)&msg_size; to_send > 0;) {
        ssize_t sent_bytes = send(co->so->client_fd, size_p, to_send, 0);
        if (sent_bytes == 0) {
            return MSG_RESULT_CLOSED;
        } else if (sent_bytes == -1) {
            return MSG_RESULT_ERROR;
        }
        to_send -= sent_bytes;
        size_p += sent_bytes;
    }

    return MSG_RESULT_SUCCESS;
}

int handle_client (struct core_object *co) {
    int recv_result = MSG_RESULT_SUCCESS;
    while (recv_result == MSG_RESULT_SUCCESS) {
        recv_result = receive_message(co);
    }
    if (recv_result == MSG_RESULT_ERROR) {
        return -1;
    } else { // connection closed
        return 0;
    }
}

static void log(struct core_object *co, struct state_object *so, ssize_t bytes,
                time_t start_time, time_t end_time, double elapsed_time_granular) {
    const size_t conn_index = 0;
    int fd;
    char *client_addr;
    in_port_t client_port;
    char *start_time_str;
    char *end_time_str;

    // NOLINTBEGIN(concurrency-mt-unsafe): No threads here
    fd = so->client_fd;
    client_addr = inet_ntoa(so->client_addr.sin_addr);
    client_port = ntohs(so->client_addr.sin_port);
    start_time_str = ctime(&start_time);
    end_time_str = ctime(&end_time);
    // NOLINTEND(concurrency-mt-unsafe)

    *(start_time_str + strlen(start_time_str) - 1) = '\0'; // Remove newline
    *(end_time_str + strlen(end_time_str) - 1) = '\0';

    /* log the connection index, the file descriptor, the client IP, the client port,
     * the number of bytes read, the start time, and the end time */
    (void) fprintf(co->log_file, "%lu,%d,%s,%d,%lu,%s,%s,%lf\n", conn_index, fd, client_addr, client_port, bytes,
                   start_time_str, end_time_str, elapsed_time_granular);
    fflush(co->log_file);

}

int destroy_state(struct state_object *so)
{
    int status;
    int big_bad_error;
    
    big_bad_error = 0;
    status = close(so->listen_fd);
    if (status == -1)
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
                big_bad_error = 1; // TODO: EIO or EINTR; not sure what to do here.
            }
        }
    }
    
    status = close(so->client_fd);
    if (status == -1)
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
                big_bad_error = 1; // TODO: EIO or EINTR; not sure what to do here.
            }
        }
    }
    
    if (big_bad_error)
    {
        return -1;
    }
    
    return 0;
}

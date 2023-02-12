#include "handle.h"

#include <log.h>
#include <util.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop" // suppress endless loop warning

/**
 * cleanup_handler
 * <p>
 * frees data on thread exit.
 * </p>
 * @param args char pointer to data.
 */
static void cleanup_handler(void *args);

void * handle(void *handle_args) {
    int server_sock;
    ssize_t result;
    clock_t  start_time_granular;
    clock_t  end_time_granular;
    ssize_t nwrote = 0;
    ssize_t nread = 0;
    uint32_t f_size;
    uint32_t resp;
    struct handle_args *h_args;

    h_args = handle_args;
    f_size = (strlen(h_args->data) + 1);
    pthread_cleanup_push(cleanup_handler, (void*)h_args->data) // run cleanup_handler on thread exit

    while (true) {
        struct logger log;
        memset(&log, 0, sizeof(struct logger));

        if (TCP_socket(&server_sock) == -1) {
            return NULL;
        }

        if (init_connection(server_sock, &h_args->server_addr) == -1) {
            log.err_msg = "could not connect to server";
        } else {
            // record start time
            log.start_time = time(NULL);
            start_time_granular = clock();

            // write file size
            uint32_t net_f_size = htonl(f_size);
            while (nwrote < (ssize_t)sizeof(net_f_size)) {
                result = write(server_sock, &net_f_size, sizeof(net_f_size));
                if (result == -1) {
                    perror("writing data size to server");
                    close_fd(server_sock);
                    return NULL;
                }
                nwrote += result;
            }

            // write file contents
            nwrote = 0;
            while (nwrote < (ssize_t)f_size) {
                result = write(server_sock, h_args->data, f_size);
                if (result == -1) {
                    perror("writing data to server");
                    close_fd(server_sock);
                    return NULL;
                }
                nwrote += result;
            }

            // receive bytes read
            while (nread < (ssize_t)sizeof(resp)) {
                result = read(server_sock, &resp, sizeof(resp));
                if (result == -1) {
                    perror("reading from server");
                    close_fd(server_sock);
                    return NULL;
                }
                nread += result;
            }

            if (close_fd(server_sock) == -1) {
                log.err_msg = "failed to close connection";
            }

            log.end_time = time(NULL);
            end_time_granular = clock();
            log.elapsed_time_granular = (double) (end_time_granular - start_time_granular) / CLOCKS_PER_SEC;
            log.server_resp = ntohl(resp);

            if (do_log(&log) == -1) {
                return NULL;
            }
        }

        pthread_testcancel();
    }

    pthread_cleanup_pop(1) // should never reach here, but set to 1 to run cleanup_handler anyway
}

static void cleanup_handler(void *args) {
        char * data;

        data = args;
        free(data);
}

#pragma clang diagnostic pop
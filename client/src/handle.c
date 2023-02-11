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
    ssize_t nwrote = 0;
    ssize_t nread = 0;
    uint32_t resp;
    struct handle_args *h_args;

    h_args = handle_args;
    pthread_cleanup_push(cleanup_handler, (void*)h_args->data) // run cleanup_handler on thread exit

    while (true) {
        struct logger log;
        memset(&log, 0, sizeof(struct logger));
        log.expected_bytes = (strlen(h_args->data) + 1);

        if (TCP_socket(&server_sock) == -1) {
            return NULL;
        }

        if (init_connection(server_sock, &h_args->server_addr) == -1) {
            log.err_msg = "could not connect to server";
        } else {
            if (set_time(&log.start_time) == -1) {
                close_fd(server_sock);
                return NULL;
            }

            while (nwrote < (ssize_t)(strlen(h_args->data) + 1)) {
                result = write(server_sock, h_args->data, (strlen(h_args->data) + 1));
                if (result == -1) {
                    perror("writing data to server");
                    close_fd(server_sock);
                    return NULL;
                }
                nwrote += result;
            }

            while (nread < (ssize_t)sizeof(resp)) {
                result = read(server_sock, &resp, sizeof(resp));
                if (result == -1) {
                    perror("reading from server");
                    close_fd(server_sock);
                    return NULL;
                }
                nread += result;
            }
            log.actual_bytes = ntohl(resp);

            if (close_fd(server_sock) == -1) {
                log.err_msg = "failed to close connection";
            }

            if (set_time(&log.end_time) == -1) {
                return NULL;
            }

            if (log_info(&log) == -1) {
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

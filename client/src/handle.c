#include "handle.h"

#include <log.h>
#include <util.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

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
            free(h_args->data);
            return NULL;
        }

        if (init_connection(server_sock, &h_args->server_addr) == -1) {
            log.err_msg = "could not connect to server";
        } else {
            log.start_time = time(NULL);

            while (nwrote < (ssize_t)(strlen(h_args->data) + 1)) {
                nwrote += write(server_sock, h_args->data, (strlen(h_args->data) + 1));
                if (nwrote == -1) {
                    perror("writing data to server");
                    if (close(server_sock) == -1) {
                        perror("closing server socket");
                    }
                    return NULL;
                }
            }

            while (nread < (ssize_t)sizeof(resp)) {
                nread += read(server_sock, &resp, sizeof(resp));
                if (nread == -1) {
                    perror("reading from server");
                    if (close(server_sock) == -1) {
                        perror("closing server socket");
                    }
                    return NULL;
                }
            }
            log.actual_bytes = ntohl(resp);

            if (close(server_sock) == -1) {
                perror("closing server socket");
                log.err_msg = "failed to close connection";
            }

            log.end_time = time(NULL);
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

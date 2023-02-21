#include "../include/state.h"
#include "connection.h"

#include <util.h>

#include <string.h>
#include <dc_application/application.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>

#define BACKLOG 10

/**
 * start_listen
 * <p>
 * creates, binds, and listens on a TCP socket.
 * </p>
 * @param s the program state.
 * @param err pointer to a dc_error struct.
 * @param env pointer to a dc_evn struct.
 * @return 0 on success. On failure, -1 and set errno.
 */
static int start_listen(struct state * s, struct dc_error * err, struct dc_env * env);

/**
 * load_data
 * <p>
 * allocates space for and reads a file.
 * </p>
 * @param dst where to allocate and store data.
 * @param size_dst where to store the data size.
 * @param file_name name of the file to read.
 * @param mode mode to open the file in.
 * @param env pointer to the dc_env struct.
 * @return 0 on success. -1 on failure and set errno.
 */
static int load_data(char **dst, off_t * size_dst, const char *file_name, const char *mode, struct dc_env * env);

/**
 * validate_params
 * <p>
 * validates the init_state_params structure.
 * </p>
 * @param params pointer to the init_state_params structure.
 * @param env pointer to the dc_env struct.
 * @return 0 if valid, -1 if invalid.
 */
static int validate_params(struct init_state_params * params, struct dc_env * env);

int init_state(struct init_state_params * params, struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    struct sockaddr_in server_addr; // only used to validate server ip and port, never accessed

    if (validate_params(params, env) == -1) return -1;

    memset(s, 0, sizeof(struct state));

    s->wait_period_sec = params->wait_period_sec;
    s->server_ip = params->server_ip;

    if (parse_port(&s->listen_port, params->listen_port, 10) == -1) return -1;
    if (parse_port(&s->server_port, params->server_port, 10) == -1) return -1;

    if (init_addr(&server_addr, s->server_ip, s->server_port) == -1) return -1; // only used for validation

    if (load_data(&s->data, &s->data_size, params->data_file_name, "r", env) == -1) return -1;

    return start_listen(s, err, env);
}

static int validate_params(struct init_state_params * params, struct dc_env * env) {
    DC_TRACE(env);

    if (params->server_ip == NULL) {
        (void) fprintf(stderr, "Server IP required, pass with -s\n");
        return -1;
    }
    if (params->data_file_name == NULL) {
        (void) fprintf(stderr, "Data file required, pass with -d\n");
        return -1;
    }
    return 0;
}

static int load_data(char **dst, off_t * size_dst, const char *file_name, const char *mode, struct dc_env * env) {
    DC_TRACE(env);
    FILE * data_file;
    struct stat data_info;
    size_t nread;
    int result;

    result = stat(file_name, &data_info);
    if (result == -1) {
        perror("stat on data file");
        return -1;
    }

    *dst = malloc(data_info.st_size);
    if (*dst == NULL) {
        perror("malloc for data");
        return -1;
    }

    if (open_file(&data_file, file_name, mode) == -1) return -1;

    nread = fread(*dst, sizeof(char), data_info.st_size, data_file);
    if (nread < (size_t)data_info.st_size) {
        if (feof(data_file))
            (void) fprintf(stderr, "reading data file: unexpected end of file\n");
        else if (ferror(data_file)) {
            perror("reading data file");
        }
        result = -1;
    }
    *size_dst = data_info.st_size;

    // close file regardless of result
    if (fclose(data_file) == -1) {
        perror("closing data file");
        result = -1;
    }

    return result;
}

static int start_listen(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    int option;

    if (init_addr_any(&s->listen_addr, s->listen_port) == -1) return -1;

    if (TCP_socket(&s->listen_fd) == -1) return -1;

    option = 1;
    setsockopt(s->listen_fd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if(bind(s->listen_fd, (struct sockaddr *)&s->listen_addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("binding listen socket");
        return -1;
    }

    if(listen(s->listen_fd, BACKLOG) == -1)
    {
        perror("listening on socket");
        return -1;
    }
    return 0;
}



int destroy_state(struct state * s, struct dc_error * err, struct dc_env * env) {
    DC_TRACE(env);
    int error = 0;

    if (s->started) {
        error = send_stop(s, err, env);
    }

    if (s->listen_fd) {
        int result = close(s->listen_fd) == -1;
        if (result == -1) {
            perror("closing listening socket");
            error = -1;
        }
    }

    for (int i = 0; i < s->num_conns; i++) {
        int result = close(s->accepted_fds[i]) == -1;
        if (result == -1) {
            perror("closing client connection");
            error = -1;
        }
    }

    if (s->data) {
        free(s->data);
    }

    return error;
}

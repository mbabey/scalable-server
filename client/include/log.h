#ifndef CLIENT_LOG_H
#define CLIENT_LOG_H

#include <state.h>

/**
 * logger
 * <p>
 * struct for holding logging information.
 * </p>
 */
struct logger {
    time_t start_time;
    time_t end_time;
    double elapsed_time_granular;
    uint32_t server_resp;
    uint32_t data_size;
    int thread_id;
};

/**
 * init_logger
 * <p>
 * opens the logging file and initializes the logging mutex.
 * </p>
 * @return 0 on success. -1 and set errno on failure.
 */
int init_logger(void);

/**
 * destroy_logger
 * <p>
 * destroys the logger mutex and closes the log file.
 * </p>
 * @return 0 on success. -1 and set errno on failure.
 */
int destroy_logger(void);

/**
 * do_log
 * <>
 * log information about the server connection.
 * </p>
 * @param l pointer to the state object.
 * @return 0 on success. -1 and set errno on failure.
 */
int do_log(struct logger * l);

#endif //CLIENT_LOG_H

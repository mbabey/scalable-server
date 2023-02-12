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
    unsigned long start_time;
    unsigned long end_time;
    uint32_t expected_bytes;
    uint32_t actual_bytes;
    const char * err_msg;
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
 * log info
 * <>
 * log information about the server connection.
 * </p>
 * @param l pointer to the state object.
 * @return 0 on success. -1 and set errno on failure.
 */
int log_info(struct logger * l);

#endif //CLIENT_LOG_H

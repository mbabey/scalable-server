#include "log.h"

/**
 * format_write
 * <p>
 * format the logging struct into a single line and append it to the log file.
 * </p>
 * @param l pointer to the logging struct to format
 * @return 0 on success. -1 and set errno on failure.
 */
static int format_write(struct logger * l);

int log_info(struct state * s, struct dc_error * err, struct dc_env * env) {
    // lock mutex

    // call format_write

    // unlock mutex

    return 0;
}

static int format_write(struct logger * l) {
    // transform struct into single line

    // write to log file
    return 0;
}

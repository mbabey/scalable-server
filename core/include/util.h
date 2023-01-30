#ifndef SCALABLE_SERVER_UTIL_H
#define SCALABLE_SERVER_UTIL_H

#include <dc_c/dc_stdio.h>

/**
 * open_file
 * <p>
 * Open a file with a given mode.
 * </p>
 * @param file_name the log file to open.
 * @param mode the mode to open the file with.
 * @return The file. NULL and set errno on failure.
 */
FILE *open_file(const char * file_name, const char * mode);

/**
 * open_lib
 * <p>
 * Open a dynamic library in a given mode.
 * </p>
 * @param lib_name name of the library to open.
 * @param mode the mode to open the library with.
 * @return The library. NULL and set errno on failure.
 */
void *open_lib(const char *lib_name, int mode);

/**
 * get_func
 * <p>
 * Get a function from a dynamic library.
 * </p>
 * @param lib the library to get from.
 * @param func_name the name of the function to get.
 * @return The function. NULL and set errno on failure.
 */
void *get_func(void *lib, const char *func_name)

#endif //SCALABLE_SERVER_UTIL_H

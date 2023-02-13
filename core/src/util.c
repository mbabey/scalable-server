#include "../include/objects.h"
#include "../include/util.h"

#include <arpa/inet.h>
#include <dlfcn.h>
#include <mem_manager/manager.h>
#include <string.h>

#define LOG_FILE_NAME "log.csv"
#define LOG_OPEN_MODE "w" // Mode is set to truncate for independent results from each experiment.

#define API_INIT "initialize_server"
#define API_RUN "run_server"
#define API_CLOSE "close_server"

/**
 * open_file
 * <p>
 * Open a file with a given mode.
 * </p>
 * @param file_name the log file to open.
 * @param mode the mode to open the file with.
 * @return The file. NULL and set errno on failure.
 */
static FILE *open_file(const char * file_name, const char * mode);

/**
 * assemble_listen_addr
 * <p>
 * Assemble a the server's listen addr. Zero memory and fill fields.
 * </p>
 * @param listen_addr the address to assemble
 * @param port_num the port number
 * @param ip_addr the IP address
 * @param mm the memory manager object
 * @return 0 on success, -1 and set errno on failure.
 */
static int assemble_listen_addr(struct sockaddr_in *listen_addr, in_port_t port_num, const char *ip_addr);

/**
 * open_lib
 * <p>
 * Open a dynamic library in a given mode.
 * </p>
 * @param lib_name name of the library to open.
 * @param mode the mode to open the library with.
 * @return The library. NULL and set errno on failure.
 */
static void *open_lib(const char *lib_name, int mode);

/**
 * get_func
 * <p>
 * Get a function from a dynamic library. Function needs to be cast to the appropriate pointer.
 * </p>
 * @param lib the library to get from.
 * @param func_name the name of the function to get.
 * @return The function. NULL and set errno on failure.
 */
static void *get_func(void *lib, const char *func_name);

void trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number)
{
    (void) fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}

int setup_core_object(struct core_object *co, const struct dc_env *env, struct dc_error *err, const in_port_t port_num,
                      const char *ip_addr)
{
    DC_TRACE(env);
    memset(co, 0, sizeof(struct core_object));
    
    co->env = env;
    co->err = err;
    co->mm  = init_mem_manager();
    if (!co->mm)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
        (void) fprintf(stderr, "Fatal: could not initialize memory manager: %s\n", strerror(errno));
        return -1;
    }
    co->log_file = open_file(LOG_FILE_NAME, LOG_OPEN_MODE);
    if (!co->log_file)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
        (void) fprintf(stderr, "Fatal: could not open %s: %s\n", LOG_FILE_NAME, strerror(errno));
        return -1;
    }
    
    if (assemble_listen_addr(&co->listen_addr, port_num, ip_addr) == -1)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
        (void) fprintf(stderr, "Fatal: could not assign server address: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

static FILE *open_file(const char *file_name, const char *mode)
{
    FILE *file;
    
    file = fopen(file_name, mode);
    // If an error occurs will return null.
    
    return file;
}

static int assemble_listen_addr(struct sockaddr_in *listen_addr, const in_port_t port_num, const char *ip_addr)
{
    int ret_val;
    
    memset(listen_addr, 0, sizeof(struct sockaddr_in));

    listen_addr->sin_port   = htons(port_num);
    listen_addr->sin_family = AF_INET;
    switch (inet_pton(AF_INET, ip_addr, &listen_addr->sin_addr.s_addr))
    {
        case 1: // Valid
        {
            ret_val = 0;
            break;
        }
        case 0: // Not a valid IP address
        {
            (void) fprintf(stderr, "%s is not a valid IP address\n", ip_addr);
            ret_val = -1;
            break;
        }
        default: // Some other error
        {
            ret_val = -1;
            break;
        }
    }
    
    return ret_val;
}

static void *open_lib(const char *lib_name, int mode)
{
    void *lib;
    
    lib = dlopen(lib_name, mode);
    // If an error occurs will return null.
    
    return lib;
}

int close_lib(void *lib, const char *lib_name)
{
    int status;
    
    status = dlclose(lib);
    if (status == -1)
    {
        // NOLINTNEXTLINE(concurrency-mt-unsafe) : No threads here
        (void) fprintf(stderr, "Fatal: could not close lib_name %s: %s\n", lib_name, strerror(errno));
    }
    
    return status;
    // If an error occurs will return -1.
}

void *get_api(struct api_functions *api, const char *lib_name, const struct dc_env *env)
{
    DC_TRACE(env);
    void *lib;
    bool get_func_err;
    
    // NOLINTBEGIN(concurrency-mt-unsafe) : No threads here
    lib = open_lib(lib_name, RTLD_LAZY);
    if (lib == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not open API library %s: %s\n", lib_name, dlerror());
        return lib;
    }
    
    get_func_err = false;
    api->initialize_server = (int (*)(struct core_object *)) get_func(lib, API_INIT);
    if (api->initialize_server == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not load API function %s: %s\n", API_INIT, strerror(errno));
        get_func_err = true;
    }
    api->run_server = (int (*)(struct core_object *)) get_func(lib, API_RUN);
    if (api->run_server == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not load API function %s: %s\n", API_RUN, strerror(errno));
        get_func_err = true;
    }
    api->close_server = (int (*)(struct core_object *)) get_func(lib, API_CLOSE);
    if (api->close_server == NULL)
    {
        (void) fprintf(stderr, "Fatal: could not load API function %s: %s\n", API_CLOSE, strerror(errno));
        get_func_err = true;
    }
    // NOLINTEND(concurrency-mt-unsafe)
    
    if (get_func_err)
    {
        close_lib(lib, NULL);
        return NULL;
    }
    
    return lib;
}

static void *get_func(void *lib, const char *func_name)
{
    void *func;
    
    func = dlsym(lib, func_name);
    // If an error occurs will return null.
    
    return func;
}

void destroy_core_object(struct core_object *co)
{
    if (co->log_file)
    {
        (void) fclose(co->log_file);
    }
    free_mem_manager(co->mm);
}

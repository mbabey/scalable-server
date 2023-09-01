#include <state.h>
#include <run.h>

#include <dc_application/application.h>
#include <dc_application/options.h>
#include <dc_c/dc_stdlib.h>
#include <dc_c/dc_string.h>
#include <dc_env/env.h>
#include <dc_error/error.h>
#include <dlfcn.h>
#include <getopt.h>
#include <mem_manager/manager.h>

#define DEFAULT_CONT_PORT "5000"
#define DEFAULT_SERVER_PORT "5000"

/**
 * application_settings
 * <p>
 * Struct of settings that control the application.
 * </p>
 */
struct application_settings
{
    struct dc_opt_settings   opts;
    struct dc_setting_string *server_ip;
    struct dc_setting_string *controller_ip;
    struct dc_setting_string *server_port;
    struct dc_setting_string *controller_port;
    struct dc_setting_string *data;
    struct dc_setting_uint16 *duration_sec;
};

/**
 * create_settings
 * <p>
 * Defines application settings and allocates memory for an application settings struct.
 * </p>
 * @param env pointer to a dc_env struct.
 * @param err pointer to a dc_error struct.
 * @return a pointer to the settings object.
 */
static struct dc_application_settings *create_settings(const struct dc_env *env, struct dc_error *err);

/**
 * run
 * <p>
 * Runs the application.
 * </p>
 * @param env pointer to a dc_env struct.
 * @param err pointer to a dc_error struct.
 * @param settings pointer to the application settings struct.
 * @return 0. -1 if an error occurred.
 */
static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings);

/**
 * destroy_settings
 * <p>
 * Frees memory allocated for the application settings struct.
 * </p>
 * @param env pointer to a dc_env struct.
 * @param err pointer to a dc_error struct.
 * @param psettings double pointer to the application settings struct to free.
 * @return 0.
 */
static int destroy_settings(const struct dc_env *env, struct dc_error *err, struct dc_application_settings **psettings);

/**
 * trace_reporter
 * <p>
 * formatting function for trace reporting.
 * </p>
 * @param env pointer to a dc_env struct
 * @param file_name name of the file the trace occurs in.
 * @param function_name name of the function the trace occurs in.
 * @param line_number the line the trace occurs in.
 */
static void
trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number);

int main(int argc, char *argv[])
{
    int                        ret_val;
    dc_env_tracer              tracer;
    struct dc_env              *env;
    struct dc_error            *err;
    struct dc_application_info *info;
    tracer = NULL;
    //tracer = trace_reporter;
    err    = dc_error_create(false);
    env    = dc_env_create(err, false, tracer);
    info   = dc_application_info_create(env, err, "client");

    ret_val = dc_application_run(env, err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle,
                                 dc_default_destroy_lifecycle, NULL, argc, argv);

    dc_application_info_destroy(env, &info);
    dc_error_reset(err);
    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_env *env, struct dc_error *err)
{
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if (settings == NULL)
    {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->server_ip               = dc_setting_string_create(env, err);
    settings->controller_ip           = dc_setting_string_create(env, err);
    settings->server_port             = dc_setting_string_create(env, err);
    settings->controller_port         = dc_setting_string_create(env, err);
    settings->data                    = dc_setting_string_create(env, err);
    settings->duration_sec            = dc_setting_uint16_create(env, err);

    struct options opts[] = {
            {(struct dc_setting *) settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'z', // set to z for controller_ip
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *) settings->server_ip,
                    dc_options_set_string,
                    "server_ip",
                    required_argument,
                    's',
                    "SERVER_IP",
                    dc_string_from_string,
                    "server_ip",
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *) settings->controller_ip,
                    dc_options_set_string,
                    "controller_ip",
                    required_argument,
                    'c',
                    "CONTROLLER_IP",
                    dc_string_from_string,
                    "controller_ip",
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *) settings->server_port,
                    dc_options_set_string,
                    "server_port",
                    required_argument,
                    'p',
                    "SERVER_PORT",
                    dc_string_from_string,
                    "server_port",
                    dc_string_from_config,
                    DEFAULT_SERVER_PORT},
            {(struct dc_setting *) settings->controller_port,
                    dc_options_set_string,
                    "controller_port",
                    required_argument,
                    'P',
                    "CONTROLLER_PORT",
                    dc_string_from_string,
                    "controller_port",
                    dc_string_from_config,
                    DEFAULT_CONT_PORT},
            {(struct dc_setting *) settings->data,
                    dc_options_set_string,
                    "data",
                    required_argument,
                    'd',
                    "DATA",
                    dc_string_from_string,
                    "data",
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *) settings->duration_sec,
                        dc_options_set_uint16,
                        "duration",
                        required_argument,
                        't',
                        "DURATION",
                        dc_uint16_from_string,
                        "duration",
                        dc_uint16_from_config,
                        0}
    };

    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size  = sizeof(struct options);
    settings->opts.opts       = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags      = "s:c:p:P:d:t:";
    settings->opts.env_prefix = "CLIENT_";

    return (struct dc_application_settings *) settings;
}

static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings)
{
    DC_TRACE(env);
    int result;
    int init_result = 0; // explicitly set to 0 for || operation
    int handle_result = 0;
    int destroy_result = 0;
    struct application_settings *app_settings;
    struct state s;
    struct init_state_params params;

    app_settings = (struct application_settings *) settings;
    params.server_ip = dc_setting_string_get(env, app_settings->server_ip);
    params.controller_ip = dc_setting_string_get(env, app_settings->controller_ip);
    params.server_port = dc_setting_string_get(env, app_settings->server_port);
    params.controller_port = dc_setting_string_get(env, app_settings->controller_port);
    params.data_file_name = dc_setting_string_get(env, app_settings->data);
    params.wait_period_sec = dc_setting_uint16_get(env, app_settings->duration_sec);

    init_result = init_state(&params, &s, err, env);
    if (init_result != -1)
    {
        handle_result = run_state(&s, err, env);
        destroy_result = destroy_state(&s, err, env);
    }
    result = init_result || handle_result || destroy_result;

    return result;
}

static int destroy_settings(const struct dc_env *env, struct dc_error *err, struct dc_application_settings **psettings)
{
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *) *psettings;
    dc_free(env, app_settings->opts.opts);
    dc_free(env, *psettings);

    if (dc_env_is_zero_free(env))
    {
        *psettings = NULL;
    }

    return 0;
}

static void
trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number)
{
    (void) fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}

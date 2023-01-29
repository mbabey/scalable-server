#include <dc_env/env.h>
#include <dc_error/error.h>
#include <dc_application/application.h>
#include <dc_application/options.h>
#include <dc_c/dc_stdlib.h>
#include <dc_c/dc_string.h>
#include <dc_c/dc_stdio.h>
#include <getopt.h>


struct application_settings {
    struct dc_opt_settings opts;
    struct dc_setting_string *library;
    // storing a struct is not possible, only use as app settings for now
};

static struct dc_application_settings *create_settings(const struct dc_env *env, struct dc_error *err);
static int destroy_settings(const struct dc_env *env, struct dc_error *err, struct dc_application_settings **psettings);
static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings);
static void trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number);

int main (int argc, char * argv[]) {
    dc_env_tracer tracer;
    struct dc_env * env;
    struct dc_error * err;
    struct dc_application_info * info;
    int ret_val;

    tracer = NULL;
    //tracer = trace_reporter;
    err = dc_error_create(false);
    env = dc_env_create(err, false, tracer);
    info = dc_application_info_create(env, err, "scalable_server");
    ret_val = dc_application_run(env, err, info, create_settings, destroy_settings, run, dc_default_create_lifecycle, dc_default_destroy_lifecycle, NULL, argc, argv);
    dc_application_info_destroy(env, &info);
    dc_error_reset(err);

    return ret_val;
}

static struct dc_application_settings *create_settings(const struct dc_env *env, struct dc_error *err) {
    struct application_settings *settings;

    DC_TRACE(env);
    settings = dc_malloc(env, err, sizeof(struct application_settings));

    if (settings == NULL) {
        return NULL;
    }

    settings->opts.parent.config_path = dc_setting_path_create(env, err);
    settings->library = dc_setting_string_create(env, err);

    struct options opts[] = {
            {(struct dc_setting *)settings->opts.parent.config_path,
                    dc_options_set_path,
                    "config",
                    required_argument,
                    'c',
                    "CONFIG",
                    dc_string_from_string,
                    NULL,
                    dc_string_from_config,
                    NULL},
            {(struct dc_setting *)settings->library,
                    dc_options_set_string,
                    "library",
                    required_argument,
                    'l',
                    "LIBRARY",
                    dc_string_from_string,
                    "library",
                    dc_string_from_config,
                    "one-to-one"}, // TODO what should the default library be?
    };

    settings->opts.opts_count = (sizeof(opts) / sizeof(struct options)) + 1;
    settings->opts.opts_size = sizeof(struct options);
    settings->opts.opts = dc_calloc(env, err, settings->opts.opts_count, settings->opts.opts_size);
    dc_memcpy(env, settings->opts.opts, opts, sizeof(opts));
    settings->opts.flags = "m:";
    settings->opts.env_prefix = "SCALABLE_SERVER_";

    return (struct dc_application_settings *)settings;
}

static int destroy_settings(const struct dc_env *env, struct dc_error *err, struct dc_application_settings **psettings) {
    struct application_settings *app_settings;

    DC_TRACE(env);
    app_settings = (struct application_settings *)*psettings;
    dc_setting_string_destroy(env, &app_settings->library);
    dc_free(env, app_settings->opts.opts);
    dc_free(env, *psettings);

    if(dc_env_is_zero_free(env))
    {
        *psettings = NULL;
    }

    return 0;
}

static int run(const struct dc_env *env, struct dc_error *err, struct dc_application_settings *settings) {
    DC_TRACE(env);
    struct application_settings *app_settings;
    int ret_val = 0;
    const char * library;

    app_settings = (struct application_settings *)settings;
    library = dc_setting_string_get(env, app_settings->library);

    // create core object

    // SETUP(library, core)
    // HANDLE(library, core)
    // DESTROY(library, core)

    return ret_val;
}

static void trace_reporter(const struct dc_env *env, const char *file_name, const char *function_name, size_t line_number) {
    fprintf(stdout, "TRACE: %s : %s : @ %zu\n", file_name, function_name, line_number);
}

#ifndef SCALABLE_SERVER_OBJECTS_H
#define SCALABLE_SERVER_OBJECTS_H

#include <stdio.h>

struct core_object {
    struct dc_env *env;
    struct dc_err *err;
    struct memory_manager *mm;
    FILE *log_file;
    struct state_object *state_obj;
};

#endif //SCALABLE_SERVER_OBJECTS_H

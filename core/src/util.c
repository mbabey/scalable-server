#include <dlfcn.h>
#include "util.h"

FILE *open_file(const char * file_name, const char * mode)
{
    FILE *file;

    file = fopen(file_name, mode);
    // If an error occurs will return null.

    return file;
}

void *open_lib(const char *lib_name, int mode)
{
    void *lib;

    lib = dlopen(lib_name, mode);
    // If an error occurs will return null.

    return lib;
}

void *get_func(void *lib, const char *func_name)
{
    void *func;

    func = dlsym(lib, func_name);
    // If an error occurs will return null.

    return func;
}

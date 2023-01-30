#include <dlfcn.h>
#include "util.h"

FILE *open_file(const char * file_name, const char * mode)
{
    FILE *file;

    file = fopen(file_name, mode);
    // If an error occurs will return null.

    return file;
}

void * open_lib(const char * lib_name, int mode)
{
    void *lib;

    lib = dlopen(lib_name, mode);
    // If an error occurs will return null.

    return lib;
}

#include "util.h"

#include <dlfcn.h>


FILE *open_file(const char * file_name, const char * mode)
{
    FILE *file;
    
    file = fopen(file_name, mode); // fixme: This is returning NULL and setting Read-only file system error
    // If an error occurs will return null.
    
    return file;
}

struct sockaddr_in *assemble_listen_addr(struct memory_manager *mm, const in_port_t port_num, const char *ip_addr)
{



}

void *open_lib(const char *lib_name, int mode)
{
    void *lib;

    lib = dlopen(lib_name, mode);
    // If an error occurs will return null.

    return lib;
}

int close_lib(void * lib) {
    return dlclose(lib);
    // If an error occurs will return null.
}

void *get_func(void *lib, const char *func_name)
{
    void *func;

    func = dlsym(lib, func_name);
    // If an error occurs will return null.

    return func;
}

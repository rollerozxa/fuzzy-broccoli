#include "misc.hh"
#include <stdlib.h>

time_t
get_mtime(const char *path)
{
    time_t mtime;
    char date[21];

    struct stat st;
    stat(path, &st);
    strftime(date, 20, "%Y-%m-%d %H:%M:%S", gmtime((time_t*)&(st.st_mtime)));
    mtime = st.st_mtime;

    return mtime;
}

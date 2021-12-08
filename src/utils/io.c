#include "io.h"
#include <ross.h>
#include <errno.h>
#include <sys/stat.h>


// Check and create directory
void check_folder(char const * const path) {
    #define NO_ERROR 0
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        int res = mkdir(path, 0700);
        if (res != NO_ERROR) {
            tw_error(TW_LOC, "Error (%d) occurred when attempting to mkdir folder `%s`", errno, path);
        }
    }
}

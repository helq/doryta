#include "utils.h"
#include <mpi.h>
#include <stdio.h>
#include <errno.h>
#include <sys/stat.h>


// Check and create directory
void check_folder(char const * const path) {
    #define NO_ERROR 0
    struct stat st = {0};

    if (stat(path, &st) == -1) {
        int res = mkdir(path, 0700);
        if (res != NO_ERROR) {
            fprintf(stderr, "Error (%d) occurred when attempting to mkdir folder `%s`.\n", errno, path);
            //exit(-1);
            MPI_Abort(MPI_COMM_WORLD, -1);
        }
    }
}

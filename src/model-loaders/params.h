#ifndef DORYTA_SRC_MODELS_PARAMS_H
#define DORYTA_SRC_MODELS_PARAMS_H

#include <stdint.h>

struct ModelParams {
    int lps_in_pe;
    unsigned long (*gid_to_pe) (uint64_t);
};

#endif /* end of include guard */

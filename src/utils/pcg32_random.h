#ifndef DORYTA_SRC_UTILS_PCG32_RANDOM_H
#define DORYTA_SRC_UTILS_PCG32_RANDOM_H

/** @file
 * Functions that extend functionality of PCG32 algorithm
 */

// This is just to not include the header (not really needed but is mostly
// snobiness from my part because I want to keep the headers as clean as
// possible)
typedef struct pcg_state_setseq_64 pcg32_random_t;


float pcg32_float_r(pcg32_random_t * rng);

#endif /* end of include guard */

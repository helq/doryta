#ifndef DORYTA_SRC_LAYOUT_STANDARD_LAYOUTS_H
#define DORYTA_SRC_LAYOUT_STANDARD_LAYOUTS_H

#include <stddef.h>

/**
 * Fully Connected Layout. The number of synapses (connections) between neurons
 * is equal to `total_neurons ^ 2`.
 */
void layout_fcn_reserve(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe);

#endif /* end of include guard */

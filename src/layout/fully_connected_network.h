#ifndef DORYTA_SRC_LAYOUT_FULLY_CONNECTED_NETWORK_H
#define DORYTA_SRC_LAYOUT_FULLY_CONNECTED_NETWORK_H

#include "../driver/neuron_lp.h"

typedef void (*neuron_init_f) (void * neuron_struct, size_t neuron_id);
typedef float (*synapse_init_f) (size_t neuron_from, size_t neuron_to);

/**
 * Reserving neuron and synapse space across a range of
 * PEs. This function does not allocate space. Only
 * layout master allocates space.
 */
void layout_fcn_reserve(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe);

/**
 * Initializing already allocated space for neurons and synapses.
 * The functions can be both NULL, in which case the
 * initialization will only link the (grouping of)
 * synapses structure correctly.
 */
void layout_fcn_init(neuron_init_f neuron_init, synapse_init_f synapse_init);

#endif /* end of include guard */

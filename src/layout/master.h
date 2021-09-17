#ifndef DORYTA_SRC_LAYOUT_MASTER_H
#define DORYTA_SRC_LAYOUT_MASTER_H

#include <stddef.h>
#include "../driver/neuron_lp.h"

enum LAYOUT_TYPE {
    LAYOUT_TYPE_fully_conn_net
};
#define NUM_LAYOUT_TYPES 1

/**
 * Reserves N neuron IDs across indicated PEs.
 * The number of neurons must be more than zero.
 * Returns the layout level at which the neurons were reserved.
 */
size_t reserve_layout_neurons(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe,
        enum LAYOUT_TYPE layoutType);

/**
 * Reserves a number of synapses on the layout level. Fails if the layout
 * level already has synapses assigned.
 * Returns the offset for the synapses given.
 */
size_t reserve_synapses_on_layout(size_t num_layout_level, size_t n_synapses);

struct MemoryAllocationLayout {
    struct Synapse           * naked_synapses;
    // The two below is what we pass to SettingsNeuronPE
    void                    ** neurons;
    struct SynapseCollection * synapses;
};

/**
 * Defines custom mapping, and allocates memory for neurons and synapses.
 *
 * Notice that the space has been allocated but not initialized! You cannot
 * simply pass `neurons` and `synapses` to `SettingsNeuronPE`!
 */
void master_layout_init(int sizeof_neuron);

/**
 * Frees memory allocated by master_layout_init
 */
void master_layout_free(void);

/**
 * Returns current allocation
 */
struct MemoryAllocationLayout memory_allocation_layout(void);

/**
 * Sets neurons and synapses into `SettingsNeuronPE`
 */

#endif /* end of include guard */

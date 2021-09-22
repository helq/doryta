#ifndef DORYTA_SRC_LAYOUT_MASTER_H
#define DORYTA_SRC_LAYOUT_MASTER_H

#include <stddef.h>
#include "../driver/neuron_lp.h"

enum LAYOUT_TYPE {
    LAYOUT_TYPE_fully_conn_net
};
#define NUM_LAYOUT_TYPES 1

struct LayoutLevelParams {
    enum LAYOUT_TYPE layoutType;
    size_t total_neurons;
    size_t initial_pe;
    size_t final_pe;
    size_t neurons_in_pe;
    size_t local_id_offset;
    size_t doryta_id_offset;
    //size_t gid_offset;
    size_t n_synapses;
    size_t synapses_offset;
};

struct MemoryAllocationLayout {
    struct Synapse           * naked_synapses;
    char                     * naked_neurons;
    // The two below is what we pass to SettingsNeuronLP
    void                    ** neurons; // In simulation time this ends up never been used, just in initialization
    struct SynapseCollection * synapses;
};


/**
 * Reserves N neuron IDs across indicated PEs.
 * The number of neurons must be more than zero.
 * Returns the layout level at which the neurons were reserved.
 */
int layout_master_reserve_neurons(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe,
        enum LAYOUT_TYPE layoutType);

/**
 * Reserves a number of synapses on the layout level. Fails if the layout
 * level already has synapses assigned.
 * Returns the offset for the synapses given.
 */
size_t layout_master_reserve_synapses(size_t num_layout_level, size_t n_synapses);

/**
 * Defines custom mapping, and allocates memory for neurons and synapses.
 *
 * Notice that the space has been allocated but not initialized! You cannot
 * simply pass `neurons` and `synapses` to `SettingsNeuronLP`!
 */
void layout_master_init(int sizeof_neuron);

/**
 * Frees memory allocated by master_layout_init
 */
void layout_master_free(void);

/**
 * Returns current neuron and synapse allocation for the layouts
 */
struct MemoryAllocationLayout
layout_master_allocation(void);

/**
 * Sets neurons and synapses into `SettingsNeuronLP`. It modifies and returns
 * the same pointer. Returning the same pointer is unnecessary but it's done so
 * that the intent of the modification is made clear when reading the code.
 * (Finding an assign operation (`=`) makes it clear that the value is being
 * changed.)
 */
struct SettingsNeuronLP *
layout_master_configure(struct SettingsNeuronLP *settingsNeuronLP);

// Supporting functions (getters)

/**
 * Returns the total number of lps (neurons and supporting lps) in this PE
 */
size_t layout_master_total_lps_pe(void);

/**
 * Returns the parameters stored in an layout level.
 */
size_t layout_master_neurons_in_pe_level(int level);

/**
 * Returns the parameters stored in an layout level.
 * Only truly available after initialization.
 */
struct LayoutLevelParams
layout_master_params_level(int level);

/**
 * Copies the each layout level configuration to an user reserved space.
 * Make sure to reserve enough space to store all layouts.
 */
void layout_master_get_all_layouts(
        struct LayoutLevelParams * to_store_in,
        enum LAYOUT_TYPE layoutType
);

size_t layout_master_doryta_id_to_gid(size_t doryta_id);

size_t layout_master_gid_to_doryta_id(size_t gid);

#endif /* end of include guard */

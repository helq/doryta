#ifndef DORYTA_SRC_LAYOUT_MASTER_H
#define DORYTA_SRC_LAYOUT_MASTER_H

#include <stddef.h>
#include "../driver/neuron_lp.h"

typedef void (*neuron_init_f) (void * neuron_struct, size_t neuron_id);
typedef float (*synapse_init_f) (size_t neuron_from, size_t neuron_to);

struct NeuronGroupInfo {
    size_t id_offset;
    size_t num_neurons;
};

/**
 * Reserves N neuron IDs across indicated PEs.
 * The number of neurons must be more than zero.
 * Returns the first doryta ID assigned to the lot.
 */
size_t layout_master_neurons(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe);

/**
 * Connects a range of neurons input (from) to a range of neurons output (to).
 * `from_start` and `from_end` identify the neurons from which a
 * connection/synapse is created, inclusive. `to_start` and `to_end` identify
 * the neuron range to connect.
 */
void layout_master_synapses_fully(size_t from_start, size_t from_end,
        size_t to_start, size_t to_end);

/**
 * Defines custom mapping, and allocates memory for neurons and synapses.
 *
 * Notice that the space has been allocated but not initialized! You cannot
 * simply pass `neurons` and `synapses` to `SettingsNeuronLP`!
 */
void layout_master_init(int sizeof_neuron,
        neuron_init_f neuron_init, synapse_init_f synapse_init);

/**
 * Frees memory allocated by master_layout_init
 */
void layout_master_free(void);

/**
 * Sets neurons and synapses into `SettingsNeuronLP`. It modifies and returns
 * the same pointer. Returning the same pointer is unnecessary but it's done so
 * that the intent of the modification is made clear when reading the code.
 * (Finding an assign operation (`=`) makes it clear that the value is being
 * changed.)
 */
struct SettingsNeuronLP *
layout_master_configure(struct SettingsNeuronLP *settingsNeuronLP);

/**
 * Returns the total number of lps (neurons and supporting lps) in this PE
 */
size_t layout_master_total_lps_pe(void);

/**
 * Returns PE to which a GID has been assigned to.
 */
unsigned long layout_master_gid_to_pe(uint64_t gid);

/**
 * Converts DorytaID into GID.
 */
size_t layout_master_doryta_id_to_gid(size_t doryta_id);

/**
 * Converts GID into DorytaID.
 */
size_t layout_master_gid_to_doryta_id(size_t gid);

/**
 * Converts LocalID into DorytaID
 */
size_t local_id_to_doryta_id(size_t id);

/**
 * Converts LocalID into DorytaID for an arbitrary PE
 */
size_t local_id_to_doryta_id_for_pe(size_t id, size_t pe);

/**
 * Returns parameters of the last group/layer defined.
 */
struct NeuronGroupInfo layout_master_info_latest_group(void);

#endif /* end of include guard */

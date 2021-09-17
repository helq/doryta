#ifndef DORYTA_SRC_LAYOUT_FULLY_CONNECTED_NETWORK_H
#define DORYTA_SRC_LAYOUT_FULLY_CONNECTED_NETWORK_H

#include "../driver/neuron_lp.h"

typedef void (*neuron_init_f) (void * neuron_struct, size_t neuron_id);
typedef float (*synapse_init_f) (size_t neuron_from, size_t neuron_to);


struct SettingsFullyConnectedNetwork {
    size_t sizeof_neuron;
    neuron_init_f neuron_init;
    synapse_init_f synapse_init;
};

static inline bool is_valid_SettingsFullyConnectedNetwork(
        struct SettingsFullyConnectedNetwork * settingsFCN
) {
    return settingsFCN->neuron_init != NULL
        && settingsFCN->synapse_init != NULL;
}

static inline void assert_valid_SettingsFullyConnectedNetwork(
        struct SettingsFullyConnectedNetwork * settingsFCN
) {
#ifndef NDEBUG
    assert(settingsFCN->neuron_init != NULL);
    assert(settingsFCN->synapse_init != NULL);
#endif // NDEBUG
}

// Reserving Neuron IDs
void reserve_fully_connected_net(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe);

// Creates the settings for a fully connected SNN with a total of
// `neurons_per_pe` per PE.
//
// Does not return a valid `struct SettingsNeuronPE`. The must to be
// completed with the necessary information
void
create_fully_connected_net(
        struct SettingsNeuronPE * settings,
        struct SettingsFullyConnectedNetwork * settingsFCN);

// Frees memory allocated when creating network
void free_fully_connected_net(struct SettingsNeuronPE *);

#endif /* end of include guard */

#include "fully_connected_network.h"
#include "master.h"
#include <ross.h>

// TODO: Get rid of this function. There are two functions now: one that maps
// ID to position in reserved neuron space (which is basically the
// porpuse of this function, and it's the identity) and GID to DGID
static size_t identity_fn_for_local_ID(size_t gid) {
    assert(g_tw_mapping == LINEAR);
    return gid - g_tw_lp_offset;
}


void reserve_fully_connected_net(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe) {
    reserve_layout_neurons(total_neurons, initial_pe, final_pe, LAYOUT_TYPE_fully_conn_net);
}


void
create_fully_connected_net(
        struct SettingsNeuronPE * settings,
        struct SettingsFullyConnectedNetwork * settingsFCN
) {

    assert_valid_SettingsFullyConnectedNetwork(settingsFCN);

    size_t neurons_in_pe = -1; // THIS IS WRONG!!
    unsigned long total_pes = tw_nnodes();
    unsigned long this_pe = g_tw_mynode;
    size_t total_neurons = neurons_in_pe * total_pes;

    settings->num_neurons = total_neurons;
    settings->num_neurons_pe = neurons_in_pe;

    // Allocating all necessary space at once
    struct SynapseCollection * synapse_collections =
        malloc(neurons_in_pe * sizeof(struct SynapseCollection));
    struct Synapse * synapses = malloc(
            neurons_in_pe * total_neurons * sizeof(struct Synapse));
    void ** pointers_to_neurons = malloc(neurons_in_pe * sizeof(void*));
    char * neurons = malloc(neurons_in_pe * settingsFCN->sizeof_neuron);

    if (synapse_collections == NULL
       || synapses == NULL
       || pointers_to_neurons == NULL
       || neurons == NULL) {
        fprintf(stderr, "Not able to allocate space.\n");
        exit(1);
    }

    for (size_t i = 0; i < neurons_in_pe; i++) {
        size_t neuron_id = neurons_in_pe * this_pe + i;

        // Initializing neuron
        void * neuron = neurons + (i * settingsFCN->sizeof_neuron);
        pointers_to_neurons[i] = neuron;
        settingsFCN->neuron_init(neuron, neuron_id);

        // Initializing synapses
        synapse_collections[i].num = total_neurons;
        synapse_collections[i].synapses = &synapses[i * total_neurons];
        struct Synapse * a_collection = synapse_collections[i].synapses;
        for (size_t j = 0; j < total_neurons; j++) {
            a_collection[j].id_to_send = j;
            a_collection[j].weight = settingsFCN->synapse_init(i, j);
        }
    }

    settings->neurons = pointers_to_neurons;
    settings->synapses = synapse_collections;

    settings->get_neuron_local_pos_init = identity_fn_for_local_ID;
}


void free_fully_connected_net(struct SettingsNeuronPE * settings) {
    free(settings->neurons[0]);  // Freeing neurons
    free(settings->neurons);  // Freeing pointers_to_neurons
    free((*settings->synapses).synapses);  // Freeing synapses
    free(settings->synapses);  // Freeing synapse_collections
}

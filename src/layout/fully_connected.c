#include "fully_connected.h"
#include <ross.h>
#include "../driver/lp_neuron.h"

static size_t identity_fn_for_ID(void * state, struct tw_lp *lp) {
    (void) state;
    return lp->gid;
}

static size_t identity_fn_for_local_ID(size_t * neurons_per_pe, struct tw_lp *lp) {
    return lp->gid % (*neurons_per_pe);
}

void
create_fully_connected(
        struct SettingsNeuronPE * settings,
        size_t sizeof_neuron,
        size_t neurons_per_pe,
        unsigned long this_pe,
        unsigned long total_pes,
        neuron_init_f neuron_init,
        synapse_init_f synapse_init) {

    assert(neuron_init != NULL);
    assert(synapse_init != NULL);

    size_t total_neurons = neurons_per_pe * total_pes;

    settings->num_neurons = total_neurons;
    settings->num_neurons_pe = neurons_per_pe;

    // Allocating all necessary space at once
    struct SynapseCollection * synapse_collections =
        malloc(neurons_per_pe * sizeof(struct SynapseCollection));
    struct Synapse * synapses = malloc(
            neurons_per_pe * total_neurons * sizeof(struct Synapse));
    void ** pointers_to_neurons = malloc(neurons_per_pe * sizeof(void*));
    char * neurons = malloc(neurons_per_pe * sizeof_neuron);

    if (synapse_collections == NULL
       || synapses == NULL
       || pointers_to_neurons == NULL
       || neurons == NULL) {
        fprintf(stderr, "Not able to allocate space.\n");
        exit(1);
    }

    for (size_t i = 0; i < neurons_per_pe; i++) {
        size_t neuron_id = neurons_per_pe * this_pe + i;

        // Initializing neuron
        void * neuron = neurons + (i * sizeof_neuron);
        pointers_to_neurons[i] = neuron;
        neuron_init(neuron, neuron_id);

        // Initializing synapses
        synapse_collections[i].num = total_neurons;
        synapse_collections[i].synapses = &synapses[i * total_neurons];
        struct Synapse * a_collection = synapse_collections[i].synapses;
        for (size_t j = 0; j < total_neurons; j++) {
            a_collection[j].id_to_send = j;
            a_collection[j].weight = synapse_init(i, j);
        }
    }

    settings->neurons = pointers_to_neurons;
    settings->synapses = synapse_collections;

    CREATE_NON_CLOSURE(
        settings->get_neuron_gid,
        get_neuron_id_c,
        identity_fn_for_ID
    );

    CREATE_CLOSURE(
        settings->get_neuron_local_pos_init,
        get_neuron_id_c,
        neurons_per_pe,
        identity_fn_for_local_ID
    );
}

void free_fully_connected(struct SettingsNeuronPE * settings) {
    DESTROY_CLOSURE(settings->get_neuron_gid);
    DESTROY_CLOSURE(settings->get_neuron_local_pos_init);

    free(settings->neurons[0]);  // Freeing neurons
    free(settings->neurons);  // Freeing pointers_to_neurons
    free((*settings->synapses).synapses);  // Freeing synapses
    free(settings->synapses);  // Freeing synapse_collections
}

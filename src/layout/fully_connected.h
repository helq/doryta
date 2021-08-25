#include "../driver/lp_neuron.h"

typedef void (*neuron_init_f) (void * neuron_struct, size_t neuron_id);
typedef float (*synapse_init_f) (size_t neuron_from, size_t neuron_to);

// Creates the settings for a fully connected SNN with a total of
// `neurons_per_pe` per PE.
//
// Does not return a valid `struct SettingsNeuronPE`. The must to be
// completed with the necessary information
void
create_fully_connected(
        struct SettingsNeuronPE * settings,
        size_t sizeof_neuron,
        size_t neurons_per_pe,
        unsigned long this_pe,
        unsigned long total_pes,
        neuron_init_f neuron_init,
        synapse_init_f synapse_init);

// Frees memory allocated when creating network
void free_fully_connected(struct SettingsNeuronPE *);

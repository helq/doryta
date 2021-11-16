#include "five_neurons.h"
#include <pcg_basic.h>
#include <ross.h>
#include "../../layout/master.h"
#include "../../layout/standard_layouts.h"
#include "../../neurons/lif.h"
#include "../../storable_spikes.h"
#include "../../utils/pcg32_random.h"


static struct StorableSpike **spikes = NULL;


static void initialize_LIF(struct LifNeuron * lif, size_t doryta_id) {
    (void) doryta_id;
    pcg32_random_t rng;
    uint32_t const initstate = doryta_id + 42u;
    uint32_t const initseq = doryta_id + 54u;
    pcg32_srandom_r(&rng, initstate, initseq);

    *lif = (struct LifNeuron) {
        .potential = 0,
        .current = 0,
        .resting_potential = 0,
        .reset_potential = 0,
        .threshold = doryta_id == 0 ? 1.2 : 0.4 + pcg32_float_r(&rng) * 0.2,
        .tau_m = .2,
        .resistance = 30
    };
}


static float initialize_weight_neurons(size_t neuron_from, size_t neuron_to) {
    (void) neuron_from;
    (void) neuron_to;

    pcg32_random_t rng;
    // Yes, we are constrained to 2^16 neurons before we start repeating
    // subsequences (there is a total of 64 bits for the generation of random
    // numbers, so 16 bits seems too little, but what happens is that there are
    // 2^32 different sequences with 2^32 elements each, precisely). Because we
    // only care in this example for one number from the sequence we have the
    // luxury of assuming that the 64bits of input are our seed. Trying to keep
    // initstate and initseq different for every weight (synapse) and neuron
    // should be enough
    uint32_t const initstate = (neuron_from + 1) + (neuron_to + 1) * 65537u + 65536u; // 2^16
    uint32_t const initseq = (neuron_from + 1) * (neuron_to + 1) + 2147483648; // 2^31
    pcg32_srandom_r(&rng, initstate, initseq);

    float const intensity = 0.1 + pcg32_float_r(&rng) * 0.52;

    return neuron_from == neuron_to ? 0 : intensity;
}


struct ModelParams
model_five_neurons_init(struct SettingsNeuronLP * settings_neuron_lp) {
    if (tw_nnodes() > 2) {
        tw_error(TW_LOC, "The five-neuron model runs on either 1 or 2 MPI Ranks, not more than that.");
    }

    // Defining Spikes
    if (g_tw_mynode <= 1) {
        spikes = calloc(5, sizeof(struct StorableSpike*));
        if (g_tw_mynode == 0) {
            spikes[0] = calloc(11, sizeof(struct StorableSpike));
            struct StorableSpike spikes_0[11] = {
                    { .neuron = 0, .time = 0.1,   .intensity = 3   },
                    { .neuron = 0, .time = 0.2,   .intensity = 1.5 },
                    { .neuron = 0, .time = 0.26,  .intensity = 1   },
                    { .neuron = 0, .time = 0.36,  .intensity = 1   },
                    { .neuron = 0, .time = 0.65,  .intensity = 1   },
                    { .neuron = 0, .time = 0.655, .intensity = 1   },
                    { .neuron = 0, .time = 0.66,  .intensity = 1   },
                    { .neuron = 0, .time = 0.67,  .intensity = 2   },
                    { .neuron = 0, .time = 0.70,  .intensity = 2   },
                    { .neuron = 0, .time = 0.71,  .intensity = 2   },
                    {0} };
            for (int i = 0; i < 10; i++) {
                spikes[0][i] = spikes_0[i];
            }
        }
        if (g_tw_mynode == 1) {
            spikes[2] = calloc(6, sizeof(struct StorableSpike));
            struct StorableSpike spikes_0[6] = {
                    { .neuron = 5, .time = 0.1,  .intensity = 1 },
                    { .neuron = 5, .time = 0.2,  .intensity = 1 },
                    { .neuron = 5, .time = 0.26, .intensity = 1 },
                    { .neuron = 5, .time = 0.36, .intensity = 1 },
                    { .neuron = 5, .time = 0.65, .intensity = 1 },
                    {0} };
            for (int i = 0; i < 5; i++) {
                spikes[2][i] = spikes_0[i];
            }
        }
    }

    // Setting the driver configuration should be done before running anything
    *settings_neuron_lp = (struct SettingsNeuronLP) {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = spikes,
      .beat              = 1.0/256,
      .firing_delay      = 1,
      .neuron_leak       = (neuron_leak_f) leak_lif_neuron,
      .neuron_leak_bigdt = (neuron_leak_big_f) leak_lif_big_neuron,
      .neuron_integrate  = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire       = (neuron_fire_f) fire_lif_neuron,
      .store_neuron         = (neuron_state_op_f) store_lif_neuron_state,
      .reverse_store_neuron = (neuron_state_op_f) reverse_store_lif_neuron_state,
      .print_neuron_struct  = (print_neuron_f) print_lif_neuron,
      //.gid_to_doryta_id    = ...
      //.probe_events     = probe_events,
    };

    // Defining layout structure (levels) and configuring neurons in current PE
    layout_std_fully_connected_network(5, 0, tw_nnodes()-1);
    if (tw_nnodes() > 1) {
        layout_std_fully_connected_layer(2, 1, 1);
    }
    // Allocates space for neurons and synapses, and initializes the neurons
    // and synapses with the given functions
    layout_master_init(sizeof(struct LifNeuron),
            (neuron_init_f) initialize_LIF,
            (synapse_init_f) initialize_weight_neurons);
    // Modifying and loading neuron configuration (it will be truly loaded
    // once the simulation starts)
    layout_master_configure(settings_neuron_lp);

    return (struct ModelParams) {
        .lps_in_pe = layout_master_total_lps_pe(),
        .gid_to_pe = layout_master_gid_to_pe,
    };
}


void model_five_neurons_deinit(void) {
    layout_master_free();

    if (g_tw_mynode == 0) {
        free(spikes[0]);
        free(spikes);
    }
    if (g_tw_mynode == 1) {
        free(spikes[2]);
        free(spikes);
    }
}

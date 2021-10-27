#include <ross.h>
#include <doryta_config.h>
#include <pcg_basic.h>
#include "driver/neuron_lp.h"
#include "layout/standard_layouts.h"
#include "layout/master.h"
#include "message.h"
//#include "neurons/lif_beta.h"
#include "neurons/lif.h"
#include "probes/firing.h"
//#include "probes/lif_beta/voltage.h"
#include "probes/lif/voltage.h"
#include "storable_spikes.h"
#include "utils/io.h"
#include "utils/pcg32_random.h"


/** Defining LP types.
 * - These are the functions called by ROSS for each LP
 * - Multiple sets can be defined (for multiple LP types)
 */
tw_lptype doryta_lps[] = {
    { // Neuron LP - needy mode
        .init     = (init_f)    neuronLP_init,
        .pre_run  = (pre_run_f) neuronLP_pre_run_needy,
        .event    = (event_f)   neuronLP_event_needy,
        .revent   = (revent_f)  neuronLP_event_reverse_needy,
        .commit   = (commit_f)  neuronLP_event_commit,
        .final    = (final_f)   neuronLP_final,
        .map      = (map_f)     NULL, // Set own mapping function. ROSS won't work without it! Use `set_mapping_on_all_lps` for that
        .state_sz = sizeof(struct NeuronLP)},

    { // Neuron LP - spike-driven mode
        .init     = (init_f)    neuronLP_init,
        .pre_run  = (pre_run_f) NULL,
        .event    = (event_f)   neuronLP_event_spike_driven,
        .revent   = (revent_f)  neuronLP_event_reverse_spike_driven,
        .commit   = (commit_f)  neuronLP_event_commit,
        .final    = (final_f)   neuronLP_final,
        .map      = (map_f)     NULL,
        .state_sz = sizeof(struct NeuronLP)},

    {0},
};

/** Define command line arguments default values. */
static bool is_spike_driven = false;


/**
 * Helper function to make all LPs use the same (GID -> local ID) mapping
 * function.
 */
static void set_mapping_on_all_lps(map_f map) {
    for (size_t i = 0; doryta_lps[i].event != NULL; i++) {
        doryta_lps[i].map = map;
    }
}


// The LP type determines the mode in which the neuron runs
static tw_lpid model_typemap(tw_lpid gid) {
    (void) gid;
    // 0 - needy mode
    // 1 - spike-driven mode
    return is_spike_driven ? 1 : 0;
}


/** Custom to doryta command line options. */
static tw_optdef const model_opts[] = {
    TWOPT_GROUP("Doryta options"),
    TWOPT_FLAG("spikedriven", is_spike_driven,
            "Activate spike-driven mode (it generally runs faster) but doesn't "
            "allow 'positive' leak"),
    TWOPT_END(),
};


//static void initialize_LIF_beta(struct LifBetaNeuron * lif, size_t doryta_id) {
//    (void) doryta_id;
//    *lif = (struct LifBetaNeuron) {
//        .potential = 0,
//        .threshold = 1.2,
//        .beta = 0.99,
//        .baseline = 0
//    };
//}


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


int main(int argc, char *argv[]) {
    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Do some error checking?
    if (g_tw_mynode == 0) {
      check_folder("output");
    }

    // Printing settings
    if (g_tw_mynode == 0) {
      printf("doryta git version: " MODEL_VERSION "\n");
    }

    // Spikes
    struct StorableSpike *spikes[5] = {
        (struct StorableSpike[]) {
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
            {0}
        },
        NULL,
        NULL,
        NULL,
        NULL
    };

    struct StorableSpike *spikes_pe1[4] = {
        NULL,
        NULL,
        (struct StorableSpike[]) {
            { .neuron = 5, .time = 0.1,  .intensity = 1 },
            { .neuron = 5, .time = 0.2,  .intensity = 1 },
            { .neuron = 5, .time = 0.26, .intensity = 1 },
            { .neuron = 5, .time = 0.36, .intensity = 1 },
            { .neuron = 5, .time = 0.65, .intensity = 1 },
            {0}
        },
        NULL,
    };

    probe_event_f probe_events[3] = {
        probes_firing_record, probes_lif_voltages_record, NULL};

    // Setting the driver configuration should be done before running anything
    struct SettingsNeuronLP settings_neuron_lp = {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = g_tw_mynode == 0 ? spikes : (g_tw_mynode == 1 ? spikes_pe1 : NULL),
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
      .probe_events     = probe_events,
    };

    // Defining layout structure (levels) and configuring neurons in current PE
    layout_std_fully_connected_network(5, 0, tw_nnodes()-1);
    if (tw_nnodes() > 1) {
        layout_std_fully_connected_network(2, 1, 1);
    }
    // Allocates space for neurons and synapses, and initializes the neurons
    // and synapses with the given functions
    layout_master_init(sizeof(struct LifNeuron),
            (neuron_init_f) initialize_LIF,
            (synapse_init_f) initialize_weight_neurons);
    // Modifying and loading neuron configuration (it will be trully loaded
    // once the simulation starts)
    settings_neuron_lp = *layout_master_configure(&settings_neuron_lp);
    neuronLP_config(&settings_neuron_lp);
    set_mapping_on_all_lps(layout_master_gid_to_pe);

    // Setting up ROSS variables
    // number of LPs == number of neurons per PE + supporting neurons
    int const num_lps_in_pe = layout_master_total_lps_pe();
    tw_define_lps(num_lps_in_pe, sizeof(struct Message));
    // to determine the type of LP
    g_tw_lp_typemap = model_typemap;
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();

    // Allocating memory for probes
    char const * const output_file =
        is_spike_driven ? "output/spike-driven-test" : "output/needy-test";
    probes_firing_init(50, output_file);
    probes_lif_voltages_init(50, output_file);

    // Running simulation
    tw_run();
    // Simulation ends when the function exits

    // Deallocating/deinitializing everything
    probes_firing_deinit();
    probes_lif_voltages_deinit();

    layout_master_free();

    tw_end();

    return 0;
}

#include <ross.h>
#include <doryta_config.h>
#include "driver/neuron_lp.h"
#include "layout/fully_connected_network.h"
#include "layout/master.h"
#include "message.h"
//#include "neurons/lif_beta.h"
#include "neurons/lif.h"
#include "probes/firing.h"
//#include "probes/lif_beta/voltage.h"
#include "probes/lif/voltage.h"
#include "storable_spikes.h"
#include "utils/io.h"


/** Defining LP types.
 * - These are the functions called by ROSS for each LP
 * - Multiple sets can be defined (for multiple LP types)
 */
tw_lptype doryta_lps[] = {
    {   .init     = (init_f)    neuronLP_init,
        .pre_run  = (pre_run_f) NULL,
        .event    = (event_f)   neuronLP_event,
        .revent   = (revent_f)  neuronLP_event_reverse,
        .commit   = (commit_f)  neuronLP_event_commit,
        .final    = (final_f)   neuronLP_final,
        .map      = (map_f)     NULL, // Set own mapping function. ROSS won't work without it! Use `set_mapping_on_all_lps` for that
        .state_sz = sizeof(struct NeuronLP)},
    {0},
};

/** Define command line arguments default values. */
// static int init_pattern = 0;

/** Custom to doryta command line options. */
//static tw_optdef const model_opts[] = {
//    TWOPT_GROUP("Doryta options"),
//    TWOPT_UINT("pattern", init_pattern, "some neuron configuration pattern"),
//    TWOPT_END(),
//};


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
    *lif = (struct LifNeuron) {
        .potential = 0,
        .current = 0,
        .resting_potential = 0,
        .reset_potential = 0,
        .threshold = 1.2,
        .tau_m = .2,
        .resistance = 30
    };
}


static float initialize_weight_neurons(size_t neuron_from, size_t neuron_to) {
    (void) neuron_from;
    (void) neuron_to;
    return neuron_from == neuron_to ? 0 : 0.4;
}


/**
 * Helper function to make all LPs use the same (GID -> local ID) mapping
 * function.
 */
static void set_mapping_on_all_lps(map_f map) {
    for (size_t i = 0; doryta_lps[i].event != NULL; i++) {
        doryta_lps[i].map = map;
    }
}


int main(int argc, char *argv[]) {
    //tw_opt_add(model_opts);
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
            {   .neuron = 0,
                .time = 0.1,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.2,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.26,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.36,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.65,
                .intensity = 1
            },
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
            {   .neuron = 5,
                .time = 0.1,
                .intensity = 1
            },
            {   .neuron = 5,
                .time = 0.2,
                .intensity = 1
            },
            {   .neuron = 5,
                .time = 0.26,
                .intensity = 1
            },
            {   .neuron = 5,
                .time = 0.36,
                .intensity = 1
            },
            {   .neuron = 5,
                .time = 0.65,
                .intensity = 1
            },
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
      .spikes           = g_tw_mynode == 0 ? spikes : (g_tw_mynode == 1 ? spikes_pe1 : NULL),
      .beat             = 1.0/256,
      .firing_delay     = 1,
      .neuron_leak      = (neuron_leak_f) leak_lif_neuron,
      .neuron_integrate = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire      = (neuron_fire_f) fire_lif_neuron,
      .store_neuron         = (neuron_state_op_f) store_lif_neuron_state,
      .reverse_store_neuron = (neuron_state_op_f) reverse_store_lif_neuron_state,
      .print_neuron_struct  = (print_neuron_f) print_lif_neuron,
      //.gid_to_doryta_id    = ...
      .probe_events     = probe_events,
    };

    // Defining layout structure (levels) and configuring neurons in current PE
    layout_fcn_reserve(5, 0, tw_nnodes()-1);
    if (tw_nnodes() > 1) {
        layout_fcn_reserve(2, 1, 1);
    }
    // Init master (allocates space for neurons and synapses)
    layout_master_init(sizeof(struct LifNeuron));
    // Initializes the neurons and synapses with the given functions
    layout_fcn_init(
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
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();

    // Allocating memory for probes
    probes_firing_init(5000);
    probes_lif_voltages_init(5000);

    // Running simulation
    tw_run();
    // Simulation ends when the function exits

    // Deallocating/deinitializing everything
    probes_firing_save("output/five-neurons-test");
    probes_firing_deinit();
    probes_lif_voltages_save("output/five-neurons-test");
    probes_lif_voltages_deinit();

    layout_master_free();

    tw_end();

    return 0;
}

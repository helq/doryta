#include <ross.h>
#include <doryta_config.h>
#include "driver/neuron_lp.h"
#include "layout/fully_connected_network.h"
#include "layout/master.h"
#include "message.h"
//#include "neurons/lif_beta.h"
#include "neurons/lif.h"
#include "probes/firing.h"
/*#include "probes/lif_beta/voltage.h"*/
#include "probes/lif/voltage.h"
#include "storable_spikes.h"
#include "utils/io.h"


static tw_peid linear_map(tw_lpid gid) {
    return (tw_peid)gid / g_tw_nlp;
}

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
        .map      = (map_f)     linear_map,
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


//static void initialize_LIF_beta(struct LifBetaNeuron * lif, size_t neuron_id) {
//    (void) neuron_id;
//    *lif = (struct LifBetaNeuron) {
//        .potential = 0,
//        .threshold = 1.2,
//        .beta = 0.99,
//        .baseline = 0
//    };
//}


static void initialize_LIF(struct LifNeuron * lif, size_t neuron_id) {
    (void) neuron_id;
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
    return 0.4;
}


int main(int argc, char *argv[]) {
    //tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // number of LPs == number of neurons per PE
    int const num_lps_in_pe = 5;

    // set up LPs within ROSS
    tw_define_lps(num_lps_in_pe, sizeof(struct Message));
    // note that g_tw_nlp gets set here by tw_define_lps

    // Do some error checking?
    if (g_tw_mynode == 0) {
      check_folder("output");
    }

    // Spikes
    struct StorableSpike *spikes[3] = {
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
        NULL
    };

    probe_event_f probe_events[3] = {
        probes_firing_record, probes_lif_voltages_record, NULL};

    // Setting the driver configuration should be done before running anything
    struct SettingsNeuronLP settings_neuron_lp = {
      //.num_neurons      = tw_nnodes() * num_lps_in_pe,
      //.num_neurons_pe   = num_lps_in_pe,
      //.neurons          = (void**) lif_neurons,
      //.synapses         = g_tw_mynode == 0 ? synapses : NULL,
      .spikes           = g_tw_mynode == 0 ? spikes : NULL,
      .beat             = 1.0/256,
      .firing_delay     = 1,
      .neuron_leak      = (neuron_leak_f) leak_lif_neuron,
      .neuron_integrate = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire      = (neuron_fire_f) fire_lif_neuron,
      .store_neuron         = (neuron_state_op_f) store_lif_neuron_state,
      .reverse_store_neuron = (neuron_state_op_f) reverse_store_lif_neuron_state,
      .print_neuron_struct  = (print_neuron_f) print_lif_neuron,
      // .get_neuron_local_pos_init = identity_fn_for_local_ID,
      .probe_events     = probe_events,
    };

    layout_fcn_reserve(5, 0, tw_nnodes()-1);

    layout_master_init(sizeof(struct LifNeuron));

    layout_fcn_init(
            (neuron_init_f) initialize_LIF,
            (synapse_init_f) initialize_weight_neurons);

    layout_master_configure(&settings_neuron_lp);
    neuronLP_config(&settings_neuron_lp);

    // Printing settings
    if (g_tw_mynode == 0) {
      printf("doryta git version: " MODEL_VERSION "\n");
    }

    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();

    // Initializing probes memory
    probes_firing_init(5000);
    probes_lif_voltages_init(5000);

    tw_run();

    // Deallocating/deinitializing everything
    probes_firing_save("output/five-neurons-test");
    probes_firing_deinit();
    probes_lif_voltages_save("output/five-neurons-test");
    probes_lif_voltages_deinit();

    layout_master_free();

    tw_end();

    return 0;
}

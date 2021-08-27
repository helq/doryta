#include <ross.h>
#include <doryta_config.h>
#include "driver/lp_neuron.h"
#include "layout/fully_connected.h"
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
tw_lptype model_lps[] = {
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
    struct StorableSpike *spikes[5] = {
        (struct StorableSpike[]) {
            {   .neuron = 0,
                .time = 0.1,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.1,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.2,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.25,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.26,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.28,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.5,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.5,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.8,
                .intensity = 1
            },
            {0}
        },
        (struct StorableSpike[]) {
            {   .neuron = 1,
                .time = 0.52,
                .intensity = 1
            },
            {   .neuron = 1,
                .time = 0.53,
                .intensity = 1
            },
            {0}
        },
        NULL,
        NULL,
        NULL
    };

    probe_event_f probe_events[3] = {
        record_firing, record_lif_voltages, NULL};

    // Setting the driver configuration should be done before running anything
    struct SettingsNeuronPE settingsNeuronPE = {
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
      // .get_neuron_gid   = identity_fn_for_ID,
      // .get_neuron_local_pos_init = identity_fn_for_local_ID,
      .probe_events     = probe_events,
    };

    create_fully_connected(
            &settingsNeuronPE,
            sizeof(struct LifNeuron),
            num_lps_in_pe,
            g_tw_mynode,
            tw_nnodes(),
            (neuron_init_f) initialize_LIF,
            (synapse_init_f) initialize_weight_neurons);

    neuron_pe_config(&settingsNeuronPE);

    // Printing settings
    if (g_tw_mynode == 0) {
      printf("doryta git version: " MODEL_VERSION "\n");
    }

    // Custom Mapping
    /*
    g_tw_mapping = CUSTOM;
    g_tw_custom_initial_mapping = &model_custom_mapping;
    g_tw_custom_lp_global_to_local_map = &model_mapping_to_lp;
    */

    // Useful ROSS variables and functions
    // tw_nnodes() : number of nodes/processors defined (ONLY available after running `tw_run`!)
    // g_tw_mynode : my node/processor id (mpi rank)

    // Useful ROSS variables (set from command line)
    // g_tw_events_per_pe
    // g_tw_lookahead
    // g_tw_nkp
    // g_tw_synchronization_protocol
    // g_tw_total_lps

    // IF there are multiple LP types
    //    you should define the mapping of GID -> lptype index
    // g_tw_lp_typemap = &model_typemap;

    // set the global variable and initialize each LP's type
    g_tw_lp_types = model_lps;
    tw_lp_setup_types();

    // Do some file I/O here? on a per-node (not per-LP) basis

    initialize_record_firing(5000);
    initialize_record_lif_voltages(5000);

    tw_run();

    // Deallocating/deinitializing everything
    save_record_firing("output/one-neuron-test");
    deinitialize_record_firing();
    save_record_lif_voltages("output/one-neuron-test");
    deinitialize_record_lif_voltages();

    free_fully_connected(&settingsNeuronPE);

    tw_end();

    return 0;
}

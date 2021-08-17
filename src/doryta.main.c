#include <ross.h>
#include <doryta_config.h>
#include "driver/lp_neuron.h"
#include "mapping.h"
#include "message.h"
#include "storable_spikes.h"
#include "probes/spikes.h"
#include "probes/lif_beta/voltage.h"
#include "neurons/lif_beta.h"
#include "utils.h"

/** Defining LP types.
 * - These are the functions called by ROSS for each LP
 * - Multiple sets can be defined (for multiple LP types)
 */
tw_lptype model_lps[] = {
    {   .init     = (init_f)    neuronLP_init,
        .pre_run  = (pre_run_f) NULL,
        .event    = (event_f)   neuronLP_event,
        .revent   = (revent_f)  NULL,
        .commit   = (commit_f)  neuronLP_event_commit,
        .final    = (final_f)   NULL,
        .map      = (map_f)     linear_map,
        .state_sz = sizeof(struct NeuronLP)},
    {0},
};

/** Define command line arguments default values. */
// static int init_pattern = 0;

/** Custom to doryta command line options. */
//static tw_optdef const model_opts[] = {
//    TWOPT_GROUP("Doryta options"),
//    TWOPT_UINT("pattern", init_pattern, "initial pattern for HighLife world"),
//    TWOPT_END(),
//};


int main(int argc, char *argv[]) {
    //tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Do some error checking?
    if (g_tw_mynode == 0) {
      check_folder("output");
    }

    struct LifNeuron *lif_neurons[1] = {
        &(struct LifNeuron) {
            .potential = 0,
            .threshhold = 1,
            .beta = 0.9,
            .baseline = 0
        }
    };

    // Synapses
    struct SynapseCollection synapses[1] = {
        {   .num = 0,
            .synapses = NULL
        }
    };

    // Spikes
    struct StorableSpike spikes_for_neuron_1[3] = {
        {   .neuron = 0,
            .time = 0.1,
            .intensity = 1
        },
        {   .neuron = 0,
            .time = 0.3,
            .intensity = 1
        },
        {0}
    };
    struct StorableSpike *spikes[1] = {
        spikes_for_neuron_1
    };

    initialize_record_spikes(5000);
    initialize_record_lif_beta_voltages(5000);
    probe_event_f probe_events[3] = {
        record_spikes, record_lif_beta_voltages, NULL};

    // Setting the driver configuration should be done before running anything
    neuron_pe_config(&(struct SettingsPE){
      .num_neurons      = 1,
      .neurons          = (void**) lif_neurons,
      .synapses         = synapses,
      .spikes           = spikes,
      .beat             = 1.0/16,
      .neuron_leak      = (neuron_leak_f) leak_lif_neuron,
      .neuron_integrate = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire      = (neuron_fire_f) fire_lif_neuron,
      .probe_events     = probe_events,
    });

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
    // tw_nnodes() : number of nodes/processors defined
    // g_tw_mynode : my node/processor id (mpi rank)

    // Useful ROSS variables (set from command line)
    // g_tw_events_per_pe
    // g_tw_lookahead
    // g_tw_nlp
    // g_tw_nkp
    // g_tw_synchronization_protocol
    // g_tw_total_lps

    // assume 1 lp in this node
    int const num_lps_in_pe = 1;

    // set up LPs within ROSS
    tw_define_lps(num_lps_in_pe, sizeof(struct Message));
    // note that g_tw_nlp gets set here by tw_define_lps

    // IF there are multiple LP types
    //    you should define the mapping of GID -> lptype index
    // g_tw_lp_typemap = &model_typemap;

    // set the global variable and initialize each LP's type
    g_tw_lp_types = model_lps;
    tw_lp_setup_types();

    // Do some file I/O here? on a per-node (not per-LP) basis

    tw_run();

    save_record_spikes("output/one-neuron-test");
    deinitialize_record_spikes();
    save_record_lif_beta_voltages("output/one-neuron-test");
    deinitialize_record_lif_beta_voltages();

    tw_end();

    return 0;
}

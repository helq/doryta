#include <ross.h>
#include <doryta_config.h>
#include "driver/lp_neuron.h"
#include "message.h"
#include "storable_spikes.h"
#include "probes/firing.h"
#include "probes/lif_beta/voltage.h"
#include "neurons/lif_beta.h"
#include "utils.h"


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


static size_t identity_fn_for_ID(struct tw_lp *lp) {
    return lp->gid;
}


static void print_LIF_beta_neuron(struct LifNeuron * lif) {
    printf("potential = %f  threshold = %f  beta = %f  baseline = %f\n",
            lif->potential, lif->threshold, lif->beta, lif->baseline);
}


int main(int argc, char *argv[]) {
    //tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Do some error checking?
    if (g_tw_mynode == 0) {
      check_folder("output");
    }

    struct LifNeuron *lif_neurons[2] = {
        &(struct LifNeuron) {
            .potential = 0,
            .threshold = 1.2,
            .beta = 0.99,
            .baseline = 0
        },
        &(struct LifNeuron) {
            .potential = .3,
            .threshold = 1,
            .beta = 0.999,
            .baseline = -0.01
        },
    };

    // Synapses
    struct SynapseCollection synapses[2] = {
        {   .num = 1,
            .synapses = (struct Synapse[1]) {
                {   .id_to_send = 1,
                    .weight = 0.5
                }
            }
        },
        {   .num = 0,
            .synapses = NULL
        }
    };

    // Spikes
    struct StorableSpike *spikes[2] = {
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
                .time = 0.15,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.5,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.75,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.75,
                .intensity = 1
            },
            {   .neuron = 0,
                .time = 0.75,
                .intensity = 1
            },
            {0}
        },
        (struct StorableSpike[]) {
            {0}
        }
    };

    initialize_record_firing(5000, &(struct FiringProbeSettings) {
      .get_neuron_id = identity_fn_for_ID
    });
    initialize_record_lif_beta_voltages(5000, &(struct VoltagesLIFbetaStngs) {
      .get_neuron_id = identity_fn_for_ID
    });
    probe_event_f probe_events[3] = {
        record_firing, record_lif_beta_voltages, NULL};

    // number of LPs == number of neurons
    int const num_lps_in_pe = 2;

    // Setting the driver configuration should be done before running anything
    neuron_pe_config(&(struct SettingsNeuronPE){
      .num_neurons      = num_lps_in_pe, // for now, it coincides with num LPs in PE
      .num_neurons_pe   = num_lps_in_pe,
      .neurons          = (void**) lif_neurons,
      .synapses         = synapses,
      .spikes           = spikes,
      .beat             = 1.0/256,
      .firing_delay     = 1,
      .neuron_init      = (neuron_init_f) NULL,
      .neuron_leak      = (neuron_leak_f) leak_lif_neuron,
      .neuron_integrate = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire      = (neuron_fire_f) fire_lif_neuron,
      .probe_events     = probe_events,
      .get_neuron_gid   = identity_fn_for_ID,
      .get_neuron_local_pos_init = identity_fn_for_ID,
      .print_neuron_struct  = (print_neuron_f) print_LIF_beta_neuron,
      .store_neuron         = (neuron_state_op_f) store_lif_neuron_state,
      .reverse_store_neuron = (neuron_state_op_f) reverse_store_lif_neuron_state,
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

    save_record_firing("output/one-neuron-test");
    deinitialize_record_firing();
    save_record_lif_beta_voltages("output/one-neuron-test");
    deinitialize_record_lif_beta_voltages();

    tw_end();

    return 0;
}

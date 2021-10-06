#include <ross.h>
#include "driver/neuron_lp.h"
#include "message.h"
#include "neurons/lif.h"
#include "probes/firing.h"
#include "probes/lif/voltage.h"
#include "storable_spikes.h"
#include "utils/io.h"

static size_t identity_map(size_t gid) {
    return gid;
}

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


int main(int argc, char *argv[]) {
    //tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Do some error checking?
    if (tw_nnodes() != 1) {
        fprintf(stderr, "This can only be run in one PE unfortunatelly :(."
                " Mapping is not properly defined!\n");
    }
    if (g_tw_mynode == 0) {
        check_folder("output");
    }

    // Neurons settings. Hardcoded by hand

    struct LifNeuron *neurons[5] = {
        &(struct LifNeuron) {  // Neuron 0
            .potential = 0,
            .current = 0,
            .resting_potential = 0,
            .reset_potential = 0,
            .threshold = 1.2,
            .tau_m = .2,
            .resistance = 30
        },
        &(struct LifNeuron) {  // Neuron 1
            .potential = 0,
            .current = 0,
            .resting_potential = 0,
            .reset_potential = 0,
            .threshold = 1.2,
            .tau_m = .2,
            .resistance = 30
        },
    };

    struct SynapseCollection synapses[5] = {
        { .num = 2,  // Neuron 0
          .synapses = (struct Synapse[2]) {
              { .gid_to_send = 0,
                .doryta_id_to_send = 0,
                .weight = .4
              },
              { .gid_to_send = 1,
                .doryta_id_to_send = 1,
                .weight = 4
              },
          }
        },
        { .num = 0,  // Neuron 1
          .synapses = NULL
        },
    };

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
    };

    probe_event_f probe_events[3] = {
        probes_firing_record, probes_lif_voltages_record, NULL};

    // Setting the driver configuration should be done before running anything
    struct SettingsNeuronLP settings_neuron_lp = {
      .num_neurons      = 2,
      .num_neurons_pe   = 2,
      .neurons          = (void**) &neurons,
      .synapses         = synapses,
      .spikes           = spikes,
      .beat             = 1.0/256,
      .firing_delay     = 1,
      .neuron_leak      = (neuron_leak_f) leak_lif_neuron,
      .neuron_integrate = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire      = (neuron_fire_f) fire_lif_neuron,
      .store_neuron         = (neuron_state_op_f) store_lif_neuron_state,
      .reverse_store_neuron = (neuron_state_op_f) reverse_store_lif_neuron_state,
      .print_neuron_struct  = (print_neuron_f) print_lif_neuron,
      .gid_to_doryta_id = identity_map,
      .probe_events     = probe_events,
    };

    neuronLP_config(&settings_neuron_lp);

    // Setting up ROSS variables
    int const num_lps_in_pe = 2;
    tw_define_lps(num_lps_in_pe, sizeof(struct Message));
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();
    // note that g_tw_nlp gets set here by tw_define_lps

    // Allocating memory for probes
    probes_firing_init(5000);
    probes_lif_voltages_init(5000);

    // Running simulation
    tw_run();
    // Simulation ends when the function exits

    // Deallocating/deinitializing everything
    probes_firing_save("output/two-neurons-test");
    probes_firing_deinit();
    probes_lif_voltages_save("output/two-neurons-test");
    probes_lif_voltages_deinit();

    tw_end();

    return 0;
}

#include <ross.h>
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


/** Defining LP types.
 * - These are the functions called by ROSS for each LP
 * - Multiple sets can be defined (for multiple LP types)
 */
tw_lptype doryta_lps[] = {
    {   .init     = (init_f)    neuronLP_init,
        .pre_run  = (pre_run_f) NULL,
        .event    = (event_f)   neuronLP_event_spike_driven,
        .revent   = (revent_f)  neuronLP_event_reverse_spike_driven,
        .commit   = (commit_f)  neuronLP_event_commit,
        .final    = (final_f)   neuronLP_final,
        .map      = (map_f)     NULL, // Set own mapping function. ROSS won't work without it! Use `set_mapping_on_all_lps` for that
        .state_sz = sizeof(struct NeuronLP)},
    {0},
};


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
    if (tw_nnodes() != 2) {
        fprintf(stderr, "This must be run in 2 PEs.");
    }
    if (g_tw_mynode == 0) {
      check_folder("output");
    }

    size_t neurons_in_pe = (10000 / tw_nnodes()) + 2;

    // Spikes
    struct StorableSpike *spikes_pe0[neurons_in_pe];
    spikes_pe0[0] = (struct StorableSpike[]) {
            {   .neuron = 0, .time = 0.1,   .intensity = 1 },
            {   .neuron = 0, .time = 0.2,   .intensity = 1 },
            {   .neuron = 0, .time = 0.26,  .intensity = 1 },
            {   .neuron = 0, .time = 0.36,  .intensity = 1 },
            {   .neuron = 0, .time = 0.65,  .intensity = 1 },
            {   .neuron = 0, .time = 0.651, .intensity = 1 },
            {   .neuron = 0, .time = 0.652, .intensity = 1 },
            {   .neuron = 0, .time = 0.653, .intensity = 1 },
            {   .neuron = 0, .time = 0.654, .intensity = 1 },
            {0} };
    for (size_t i = 1; i < neurons_in_pe; i++) {
        spikes_pe0[i] = NULL;
    }

    probe_event_f probe_events[3] = {
        probes_firing_record, probes_lif_voltages_record, NULL};

    // Setting the driver configuration should be done before running anything
    struct SettingsNeuronLP settings_neuron_lp = {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = g_tw_mynode == 0 ? spikes_pe0 : NULL,
      .beat              = 1.0/256,
      .firing_delay      = 1,
      .neuron_leak       = (neuron_leak_f) leak_lif_neuron,
      .neuron_leak_bigdt = (neuron_leak_big_f) leak_lif_big_neuron,
      .neuron_integrate  = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire       = (neuron_fire_f) fire_lif_neuron,
      .store_neuron         = (neuron_state_op_f) store_lif_neuron_state,
      .reverse_store_neuron = (neuron_state_op_f) reverse_store_lif_neuron_state,
      //.print_neuron_struct  = (print_neuron_f) print_lif_neuron,
      //.gid_to_doryta_id    = ...
      .probe_events     = probe_events,
    };

    // Defining layout structure (levels) and configuring neurons in current PE
    layout_std_fully_connected_network(10000, 0, tw_nnodes()-1);
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
    // number of LPs == number of neurons per PE
    int const num_lps_in_pe = layout_master_total_lps_pe();
    tw_define_lps(num_lps_in_pe, sizeof(struct Message));
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();
    // note that g_tw_nlp gets set here by tw_define_lps

    // Allocating memory for probes
    probes_firing_init(5000);
    probes_lif_voltages_init(50000);

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

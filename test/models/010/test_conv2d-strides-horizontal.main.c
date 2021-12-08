#include <ross.h>
#include <doryta_config.h>
#include "driver/neuron.h"
#include "layout/standard_layouts.h"
#include "layout/master.h"
#include "message.h"
#include "neurons/lif.h"
#include "probes/firing.h"
#include "probes/lif/voltage.h"
#include "storable_spikes.h"
#include "utils/io.h"


/** Defining LP types.
 * - These are the functions called by ROSS for each LP
 * - Multiple sets can be defined (for multiple LP types)
 */
tw_lptype doryta_lps[] = {
    { // Neuron LP - needy mode
        .init     = (init_f)    driver_neuron_init,
        .pre_run  = (pre_run_f) driver_neuron_pre_run_needy,
        .event    = (event_f)   driver_neuron_event_needy,
        .revent   = (revent_f)  driver_neuron_event_reverse_needy,
        .commit   = (commit_f)  driver_neuron_event_commit,
        .final    = (final_f)   driver_neuron_final,
        .map      = (map_f)     NULL, // Set own mapping function. ROSS won't work without it! Use `set_mapping_on_all_lps` for that
        .state_sz = sizeof(struct NeuronLP)},

    { // Neuron LP - spike-driven mode
        .init     = (init_f)    driver_neuron_init,
        .pre_run  = (pre_run_f) NULL,
        .event    = (event_f)   driver_neuron_event_spike_driven,
        .revent   = (revent_f)  driver_neuron_event_reverse_spike_driven,
        .commit   = (commit_f)  driver_neuron_event_commit,
        .final    = (final_f)   driver_neuron_final,
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
    TWOPT_FLAG("spike-driven", is_spike_driven,
            "Activate spike-driven mode (it generally runs faster) but doesn't "
            "allow 'positive' leak"),
    TWOPT_END(),
};


static void initialize_LIF(struct LifNeuron * lif, int32_t doryta_id) {
    (void) doryta_id;

    *lif = (struct LifNeuron) {
        .potential = 0,
        .current = 0,
        .resting_potential = 0,
        .reset_potential = 0,
        .threshold = 100,
        .tau_m = 131072, // Large tau mean that the neuron has long memory
        .resistance = 131072 * 256
    };
}


int main(int argc, char *argv[]) {
    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Do some error checking?
    if (g_tw_mynode == 0) {
      check_folder("output");
    }

    MPI_Barrier(MPI_COMM_ROSS);
    unsigned long const self = g_tw_mynode;
    // Finding name for file
    char const fmt[] = "output/conv2d-strides-final-state-gid=%lu.txt";
    int const sz = snprintf(NULL, 0, fmt, self);
    char filename_path[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename_path, sizeof(filename_path), fmt, self);

    FILE * fh = fopen(filename_path, "w");

    if (fh == NULL) {
        tw_error(TW_LOC, "Unable to store final state in file %s\n", filename_path);
    }

    probe_event_f probe_events[3] = {
        probes_firing_record, probes_lif_voltages_record, NULL};

    // Setting the driver configuration
    struct SettingsNeuronLP settings_neuron_lp = {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = NULL,
      .beat              = 1.0/256,
      .firing_delay      = 1,
      .neuron_leak       = (neuron_leak_f) neurons_lif_leak,
      .neuron_leak_bigdt = (neuron_leak_big_f) neurons_lif_big_leak,
      .neuron_integrate  = (neuron_integrate_f) neurons_lif_integrate,
      .neuron_fire       = (neuron_fire_f) neurons_lif_fire,
      .store_neuron         = (neuron_state_op_f) neurons_lif_store_state,
      .reverse_store_neuron = (neuron_state_op_f) neurons_lif_reverse_store_state,
      .print_neuron_struct  = (print_neuron_f) neurons_lif_print,
      //.gid_to_doryta_id    = ...
      .probe_events     = probe_events,
      .save_state_handler = fh,
    };

    unsigned long const last_node = tw_nnodes()-1;
    // Defining neurons
    // GoL board
    layout_master_neurons(420, 0, last_node);  // 20 x 21
    // Fire if life (either birth or sustain life)
    layout_master_neurons(28, 0, last_node); // 4 x 7

    // Defining synapses
    struct Conv2dParams const conv2d_params = {
            .input_width    = 20,
            .kernel_width   = 5,
            .kernel_height  = 3,
            .padding_width  = 0,
            .padding_height = 0,
            .stride_width  = 5,
            .stride_height = 3,
    };
    layout_master_synapses_conv2d(0, 419, 420, 420 + 28-1,  &conv2d_params);

    // Allocates space for neurons and synapses, and initializes the neurons
    // and synapses with the given functions
    layout_master_init(sizeof(struct LifNeuron),
            (neuron_init_f) initialize_LIF,
            (synapse_init_f) NULL);
    // Modifying and loading neuron configuration (it will be truly loaded
    // once the simulation starts)
    layout_master_configure(&settings_neuron_lp);

    // Defining spikes
    struct StorableSpike **spikes = NULL;

    if (g_tw_mynode == 0) {
        spikes = calloc(settings_neuron_lp.num_neurons_pe, sizeof(struct StorableSpike*));
        spikes[0] = calloc(4, sizeof(struct StorableSpike));
        spikes[0][0] = (struct StorableSpike){
            .neuron = 0, .time = 0.0, .intensity = 101 };
        spikes[1] = spikes[0] + 2;
        spikes[1][0] = (struct StorableSpike){
            .neuron = 1, .time = 0.6, .intensity = 101 };
    }

    settings_neuron_lp.spikes = spikes;

    // Final setup
    driver_neuron_config(&settings_neuron_lp);
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
    probes_firing_init(5000, "output", "conv2d-strides", false);
    probes_lif_voltages_init(5000, "output", "conv2d-strides");

    // Running simulation
    tw_run();
    // Simulation ends when the function exits

    // Deallocating/deinitializing everything
    if (fh != NULL) {
        fclose(fh);
    }
    if (g_tw_mynode == 0) {
        free(spikes[0]);
        free(spikes);
    }
    probes_firing_deinit();
    probes_lif_voltages_deinit();

    layout_master_free();

    tw_end();

    return 0;
}

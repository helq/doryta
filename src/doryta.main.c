#include <ross.h>
#include <doryta_config.h>
#include "driver/neuron_lp.h"
#include "message.h"
#include "models/params.h"
#include "models/simple_example.h"
#include "probes/firing.h"
#include "probes/lif/voltage.h"
#include "utils/io.h"


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
static bool run_simple_example = false;
//static char * output_dir = "output";


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
    TWOPT_GROUP("Doryta General Options"),
    TWOPT_FLAG("spikedriven", is_spike_driven,
            "Activate spike-driven mode (it generally runs faster) but doesn't "
            "allow 'positive' leak"),
    //TWOPT_CHAR("outputdir", output_dir,
    //        "Path to store the output of a model execution"),
    TWOPT_GROUP("Doryta Models"),
    TWOPT_FLAG("simple-example", run_simple_example,
            "Run a simple 5 (or 7) neurons network example (useful to check "
            "the binary is working properly)"),
    TWOPT_END(),
};


int main(int argc, char *argv[]) {
    //output_dir = calloc(101, sizeof(char));
    //{
    //    char const * o_f = "output";
    //    // basically strcpy
    //    for (int i = 0; i < 100 && o_f[i] != '\0'; i++) {
    //        output_dir[i] = o_f[i];
    //    }
    //}

    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    if (g_tw_mynode == 0) {
      printf("doryta git version: " MODEL_VERSION "\n");
    }

    // ------ Checking arguments correctness and misc checks ------
    if (g_tw_mynode == 0) {
        check_folder("output");
    }
    if (!run_simple_example) {
        tw_error(TW_LOC, "You have to specify a model to run");
    }

    // --------------- Allocating model and probes ----------------
    struct SettingsNeuronLP settings_neuron_lp;
    struct ModelParams params = model_simple_example_init(&settings_neuron_lp);

    probe_event_f probe_events[3] = {
        probes_firing_record, probes_lif_voltages_record, NULL};
    settings_neuron_lp.probe_events = probe_events;

    // ---------------------- Setting up LPs ----------------------
    neuronLP_config(&settings_neuron_lp);

    // ---------------- Setting up ROSS variables -----------------
    set_mapping_on_all_lps(params.gid_to_pe);
    tw_define_lps(params.lps_in_pe, sizeof(struct Message));
    // to determine the type of LP
    g_tw_lp_typemap = model_typemap;
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();

    // --------------- Allocating memory for probes ---------------
    probes_firing_init(5000);
    probes_lif_voltages_init(5000);

    // -------------------- Running simulation --------------------
    tw_run();

    // ----------------- Saving results of probes -----------------
    probes_firing_save("output/five-neurons-test");
    probes_lif_voltages_save("output/five-neurons-test");

    // -------------- Deallocating model and probes ---------------
    probes_firing_deinit(); // probes store data on deinit
    probes_lif_voltages_deinit();
    model_simple_example_deinit();

    tw_end();

    return 0;
}

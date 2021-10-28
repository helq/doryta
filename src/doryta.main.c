#include <ross.h>
#include <doryta_config.h>
#include "driver/neuron_lp.h"
#include "message.h"
#include "models/params.h"
#include "models/hardcoded/five_neurons.h"
#include "models/regular_io/load_neurons.h"
#include "models/regular_io/load_spikes.h"
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
static bool run_five_neuron_example = false;
static char output_dir[512] = "output"; // UNSAFE but the only way to do it!!
static char model_path[512] = "\0";
static char spikes_path[512] = "\0";


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
    TWOPT_CHAR("outputdir", output_dir,
            "Path to store the output of a model execution"),
    // TODO: add options for each probe and their params
    //TWOPT_GROUP("Doryta Probes"),
    //TWOPT_FLAG("tag", XXX, "description"),
    TWOPT_GROUP("Doryta Models"),
    TWOPT_CHAR("load-model", model_path,
            "Load model from file"),
    TWOPT_FLAG("five-example", run_five_neuron_example,
            "Run a simple 5 (or 7) neurons network example (useful to check "
            "the binary is working properly)"),
    TWOPT_GROUP("Doryta Spikes"),
    TWOPT_CHAR("load-spikes", spikes_path,
            "Load spikes from file"),
    TWOPT_END(),
};


int main(int argc, char *argv[]) {
    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    if (g_tw_mynode == 0) {
      printf("doryta git version: " MODEL_VERSION "\n");
    }

    // ------ Checking arguments correctness and misc checks ------
    if (g_tw_mynode == 0) {
        if (output_dir[0] == '\0') {
            tw_error(TW_LOC, "Output directory name is empty");
        }
        check_folder(output_dir);
    }
    int const num_models_selected = run_five_neuron_example + (model_path[0] != '\0');
    if (num_models_selected != 1) {
        if (g_tw_mynode == 0) {
            fprintf(stderr, "Total loading model options selected: %d\n",
                    num_models_selected);
            fprintf(stderr, "Five neurons example: %s\n",
                    run_five_neuron_example ? "ON" : "OFF");
            fprintf(stderr, "Path to load model: '%s'\n", model_path);
        }
        tw_error(TW_LOC, "You have to specify ONE model to run");
    }

    // --------------- Allocating model and probes ----------------
    struct SettingsNeuronLP settings_neuron_lp;
    struct ModelParams params;

    if (run_five_neuron_example) {
        params = model_five_neurons_init(&settings_neuron_lp);
    }
    if (model_path[0] != '\0') {
        params = model_load_neurons_init(&settings_neuron_lp, model_path);
    }

    if (spikes_path[0] != '\0') {
        model_load_spikes_init(&settings_neuron_lp, spikes_path);
    }

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
    probes_firing_init(5000, "output/five-neurons-test");
    probes_lif_voltages_init(5000, "output/five-neurons-test");

    // -------------------- Running simulation --------------------
    tw_run();

    // -------------- Deallocating model and probes ---------------
    probes_firing_deinit(); // probes store data on deinit
    probes_lif_voltages_deinit();

    if (run_five_neuron_example) {
        model_five_neurons_deinit();
    }

    tw_end();

    return 0;
}

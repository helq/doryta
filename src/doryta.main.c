#include <ross.h>
#include <doryta_config.h>
#include "driver/neuron.h"
#include "model-loaders/hardcoded/five_neurons.h"
#include "model-loaders/hardcoded/gameoflife.h"
#include "model-loaders/regular_io/load_neurons.h"
#include "model-loaders/regular_io/load_spikes.h"
#include "probes/firing.h"
#include "probes/stats.h"
#include "probes/lif/voltage.h"
#include "utils/io.h"
#include "version.h"


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
// Bools
// NOTE: bools cannot be of type bool because ROSS assumes all arguments to be
// ints except for chars
static unsigned int is_spike_driven = 0;
static unsigned int run_five_neuron_example = 0;
static unsigned int gol = 0;
static unsigned int is_firing_probe_active = 0;
static unsigned int is_voltage_probe_active = 0;
static unsigned int is_stats_probe_active = 0;
static unsigned int probe_firing_output_neurons_only = 0;
static unsigned int save_final_state_neurons = 0;
// Ints
static unsigned int probe_firing_buffer_size = 5000;
static unsigned int probe_voltage_buffer_size = 5000;
// Strings
// Yes, caping the size to 512 is UNSAFE but the only way to do it!!
static char output_dir[512] = "output";
static char model_path[512] = {'\0'};
static char spikes_path[512] = {'\0'};
static char model_filename[512] = "model-name";


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
    TWOPT_FLAG("spike-driven", is_spike_driven,
            "Activate spike-driven mode (it generally runs faster) but doesn't "
            "allow 'positive' leak"),
    TWOPT_CHAR("output-dir", output_dir,
            "Path to store the output of a model execution"),
    TWOPT_CHAR("model-filename", model_filename, "Model file name. To be used by all output"),
    TWOPT_FLAG("save-state", save_final_state_neurons,
            "Saves the final state (after simulation) of neurons (in spike-driven mode "
            "the final state will be that in which the neuron was after it received the last spike)"),
    TWOPT_GROUP("Doryta Models"),
    TWOPT_CHAR("load-model", model_path,
            "Load model from file"),
    TWOPT_FLAG("five-example", run_five_neuron_example,
            "Run a simple 5 (or 7) neurons network example (useful to check "
            "the binary is working properly)"),
    TWOPT_FLAG("gol-model", gol,
            "Defines a 20x20 Game Of Life grid using Conv2D layers"),
    TWOPT_GROUP("Doryta Spikes"),
    TWOPT_CHAR("load-spikes", spikes_path,
            "Load spikes from file"),
    TWOPT_GROUP("Doryta Probes"),
    TWOPT_FLAG("probe-firing", is_firing_probe_active,
            "This probe records when a neuron fires/sends a spike"),
    TWOPT_FLAG("probe-firing-output-only", probe_firing_output_neurons_only,
            "Record only neurons with no output synapses (these often are the last layer in a "
            "multilayer NN)"),
    TWOPT_UINT("probe-firing-buffer", probe_firing_buffer_size,
            "Buffer size for firing probe. If the buffer fills, nothing else will stored in it"),
    TWOPT_FLAG("probe-voltage", is_voltage_probe_active,
            "This probe records the voltage of a LIF neuron as it changes in "
            "time (won't work as expected with spike-driven activated)"),
    TWOPT_UINT("probe-voltage-buffer", probe_voltage_buffer_size,
            "Buffer size for firing probe. If the buffer fills, nothing else will stored in it"),
    TWOPT_FLAG("probe-stats", is_stats_probe_active,
            "This probe records basic stats for each neuron (number of leak, "
            "integrate and fire operations)"),
    TWOPT_END(),
};


void fprint_doryta_params(FILE * fp) {
    fprintf(fp, "doryta version: " DORYTA_VERSION "-" GIT_VERSION "\n");
    fprintf(fp, "=============== Params passed to doryta ===============\n");
    fprintf(fp, "spike-driven          = %s\n",   is_spike_driven ? "ON" : "OFF");
    fprintf(fp, "output-dir            = '%s'\n", output_dir);
    fprintf(fp, "model-filename        = '%s'\n", model_filename);
    fprintf(fp, "save-state            = %s\n",   save_final_state_neurons ? "ON" : "OFF");
    fprintf(fp, "load-model            = '%s'\n", model_path);
    fprintf(fp, "five-example          = %s\n",   run_five_neuron_example ? "ON" : "OFF");
    fprintf(fp, "gol                   = %s\n",   gol ? "ON" : "OFF");
    fprintf(fp, "load-spikes           = '%s'\n", spikes_path);
    fprintf(fp, "probe-firing          = %s\n",   is_firing_probe_active ? "ON" : "OFF");
    fprintf(fp, "probe-firing-output-only = %s\n", probe_firing_output_neurons_only ? "ON" : "OFF");
    fprintf(fp, "probe-firing-buffer   = %d\n",   probe_firing_buffer_size);
    fprintf(fp, "probe-voltage         = %s\n",   is_voltage_probe_active ? "ON" : "OFF");
    fprintf(fp, "probe-voltage-buffer  = %d\n",   probe_voltage_buffer_size);
    fprintf(fp, "probe-stats           = %s\n",   is_stats_probe_active ? "ON" : "OFF");
    fprintf(fp, "=======================================================\n");
}


int main(int argc, char *argv[]) {
    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    if (g_tw_mynode == 0) {
        printf("\n");
        fprint_doryta_params(stdout);
        printf("\n");
    }

    // ------ Checking arguments correctness, save params and misc checks ------
    if (g_tw_mynode == 0) {
        // Checking correctness
        if (output_dir[0] == '\0') {
            tw_error(TW_LOC, "Output directory name is empty");
        }
        check_folder(output_dir);

        // Saving params to file
        int sz = snprintf(NULL, 0, "%s/doryta-params.txt", output_dir);
        char filename[sz + 1];
        snprintf(filename, sizeof(filename), "%s/doryta-params.txt", output_dir);

        FILE * fp = fopen(filename, "w");
        if (fp != NULL) {
            fprint_doryta_params(fp);
            fclose(fp);
        } else {
            tw_error(TW_LOC, "Cannot save doryta configuration to file `%s`. "
                    "Make sure that the directory is reachable and writable", filename);
        }
    }
    int const num_models_selected = gol + run_five_neuron_example + (model_path[0] != '\0');
    if (num_models_selected != 1) {
        tw_error(TW_LOC, "You have to specify ONE model to run");
    }

    // ------------- Initializing model, spikes and probes (partially) -------------
    struct SettingsNeuronLP settings_neuron_lp;
    struct ModelParams params;

    // Loading Model
    if (run_five_neuron_example) {
        params = model_five_neurons_init(&settings_neuron_lp);
    }
    if (gol) {
        params = model_GoL_neurons_init(&settings_neuron_lp);
    }
    if (model_path[0] != '\0') {
        params = model_load_neurons_init(&settings_neuron_lp, model_path);
    }

    // Loading Spikes
    if (spikes_path[0] != '\0') {
        model_load_spikes_init(&settings_neuron_lp, spikes_path);
    }

    // Saving neuron states after execution
    if (save_final_state_neurons) {
        unsigned long const self = g_tw_mynode;
        // Finding name for file
        char const fmt[] = "%s/%s-final-state-gid=%lu.txt";
        int const sz = snprintf(NULL, 0, fmt, output_dir, model_filename, self);
        char filename_path[sz + 1]; // `+ 1` for terminating null byte
        snprintf(filename_path, sizeof(filename_path), fmt, output_dir, model_filename, self);

        // This is needed because there might be race condition when creating
        // the output folder (mkdir'd by PE=0)
        MPI_Barrier(MPI_COMM_ROSS);

        FILE * fh = fopen(filename_path, "w");
        if (fh == NULL) {
            fprintf(stderr, "Unable to store final state in file %s."
                    "Check that you have write permissions.\n", filename_path);
        }

        settings_neuron_lp.save_state_handler = fh;
    }

    // Loading probe recording mechanism
    probe_event_f probe_events[4] = {NULL};
    {
        int i = 0;
        if (is_firing_probe_active) {
            probe_events[i] = probes_firing_record;
            i++;
        }
        if (is_voltage_probe_active) {
            probe_events[i] = probes_lif_voltages_record;
            i++;
        }
        if (is_stats_probe_active) {
            probe_events[i] = probes_stats_record;
            i++;
        }
        probe_events[i] = NULL;
    }
    settings_neuron_lp.probe_events = probe_events;

    // ---------------------- Setting up LPs ----------------------
    driver_neuron_config(&settings_neuron_lp);

    // ---------------- Setting up ROSS variables -----------------
    set_mapping_on_all_lps(params.gid_to_pe);
    tw_define_lps(params.lps_in_pe, sizeof(struct Message));
    // to determine the type of LP
    g_tw_lp_typemap = model_typemap;
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();

    // --------------- Allocating memory for probes ---------------
    if (is_firing_probe_active) {
        probes_firing_init(probe_firing_buffer_size, output_dir, model_filename,
                probe_firing_output_neurons_only);
    }
    if (is_voltage_probe_active) {
        probes_lif_voltages_init(probe_voltage_buffer_size, output_dir, model_filename);
    }
    if (is_stats_probe_active) {
        probes_stats_init(settings_neuron_lp.num_neurons_pe, output_dir, model_filename);
    }

    // -------------------- Running simulation --------------------
    tw_run();

    // -------------- Deallocating model and probes ---------------
    // --- DeInit of Probes ---
    if (is_firing_probe_active) {
        probes_firing_deinit(); // probes store data on deinit
    }
    if (is_voltage_probe_active) {
        probes_lif_voltages_deinit();
    }
    if (is_stats_probe_active) {
        probes_stats_deinit();
    }

    // --- DeInit of Spikes ---
    if (save_final_state_neurons) {
        if (settings_neuron_lp.save_state_handler != NULL) {
            fclose(settings_neuron_lp.save_state_handler);
        }
    }

    // --- DeInit of Spikes ---
    if (spikes_path[0] != '\0') {
        model_load_spikes_deinit();
    }

    // --- DeInit of Neurons ---
    if (run_five_neuron_example) {
        model_five_neurons_deinit();
    }
    if (model_path[0] != '\0') {
        model_load_neurons_deinit();
    }

    tw_end();

    return 0;
}

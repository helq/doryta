/**
 * This is a regression test for a bug resulting from incorrect/incomplete
 * reverse computation. When `Message` is a heartbeat, `fired` is set to
 * false, and so it should stay unless the function "leak" tells it that it
 * must spike. In case a heartbeat is rolled back and it had fired/spiked,
 * the value stored in `Message` must be set back to false, but it was never
 * done so in the code. This bug provokes occassional spikes on the output,
 * but the computation runs correctly, most of the time (same number of net
 * events are processed).
 */

#include <ross.h>
#include <doryta_config.h>
#include "driver/neuron.h"
#include "message.h"
#include "probes/firing.h"
#include "storable_spikes.h"
#include "utils/io.h"
#include <time.h>


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
static int total_spikes = 100;


/**
 * Helper function to make all LPs use the same (GID -> local ID) mapping
 * function.
 */
static void set_mapping_on_all_lps(map_f map) {
    for (size_t i = 0; doryta_lps[i].event != NULL; i++) {
        doryta_lps[i].map = map;
    }
}


static tw_peid linear_map(tw_lpid gid) {
    return (tw_peid)gid / g_tw_nlp;
}


static int32_t identity_map(size_t gid) {
    return gid;
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
    TWOPT_UINT("total-spikes", total_spikes,
            "Total individual input spikes"),
    TWOPT_END(),
};


struct DummyNeuron {
    int32_t potential;
    int32_t threshold;
};


static void dummy_leak(struct DummyNeuron * dn, double d_time) {
    (void) dn;
    (void) d_time;
}

static void big_dummy_leak(struct DummyNeuron * dn, double d_time, double dt) {
    dn->potential = 0;
    (void) d_time;
    (void) dt;
}


static void dummy_integrate(struct DummyNeuron * dn, float intensity) {
    (void) intensity;
    dn->potential += (int)intensity;
}


static bool dummy_fire(struct DummyNeuron * dn) {
    bool const to_fire = dn->potential > dn->threshold;
    return to_fire;
}


static void dummy_store(struct DummyNeuron * dn, struct DummyNeuron * in_msg) {
    in_msg->potential = dn->potential;
}


static void dummy_restore(struct DummyNeuron * dn, struct DummyNeuron * in_msg) {
    dn->potential = in_msg->potential;
}


//static void dummy_print(struct DummyNeuron * dn) {
//    printf("potential = %d  threshold = %d\n", dn->potential, dn->threshold);
//}


int main(int argc, char *argv[]) {
    tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Do some error checking?
    if (g_tw_mynode == 0) {
      check_folder("output");
    }

    // This must be run in two PEs precisely
    if (tw_nnodes() != 2) {
        tw_error(TW_LOC, "This model must be run on two PEs");
    }

    int const num_lps_in_pe = 2;

    struct DummyNeuron * neurons[2] = {
        &(struct DummyNeuron) {
            .potential = 0,
            .threshold = 0,
        },
        &(struct DummyNeuron) {
            .potential = 0,
            .threshold = 0,
        },
    };

    struct SynapseCollection synapses[2] = {
        { .num = 1,  // Neuron 0
          .synapses = (struct Synapse[1]) {
              { .gid_to_send = 1,
                .doryta_id_to_send = 1,
                .weight = g_tw_mynode == 0 ? 1 : -1
              },
          }
        },
        { .num = 0,  // Neuron 1
          .synapses = NULL
        },
    };

    struct StorableSpike *spikes[2] = { 0 };
    spikes[0] = calloc(total_spikes + 1, sizeof(struct StorableSpike));

    for (int i = 0; i < total_spikes; i++) {
        spikes[0][i] = (struct StorableSpike) {
            .neuron    = g_tw_mynode == 0 ? 0 : 2,
            .time      = i + 0.1,
            .intensity = 1
        };
    }

    probe_event_f probe_events[2] = {probes_firing_record, NULL};

    // Setting the driver configuration should be done before running anything
    struct SettingsNeuronLP settings_neuron_lp = {
      .num_neurons      = 4,
      .num_neurons_pe   = 2,
      .neurons          = (void**) neurons,
      .synapses         = synapses,
      .spikes            = spikes,
      .beat              = 1.0/256,
      .firing_delay      = 1,
      .neuron_leak       = (neuron_leak_f) dummy_leak,
      .neuron_leak_bigdt = (neuron_leak_big_f) big_dummy_leak,
      .neuron_integrate  = (neuron_integrate_f) dummy_integrate,
      .neuron_fire       = (neuron_fire_f) dummy_fire,
      .store_neuron         = (neuron_state_op_f) dummy_store,
      .reverse_store_neuron = (neuron_state_op_f) dummy_restore,
      //.print_neuron_struct  = (print_neuron_f) dummy_print,
      .gid_to_doryta_id = identity_map,
      .probe_events     = probe_events,
    };

    // Modifying and loading neuron configuration (it will be trully loaded
    // once the simulation starts)
    driver_neuron_config(&settings_neuron_lp);
    set_mapping_on_all_lps(linear_map);

    // Setting up ROSS variables
    // number of LPs == number of neurons per PE + supporting neurons
    tw_define_lps(num_lps_in_pe, sizeof(struct Message));
    // to determine the type of LP
    g_tw_lp_typemap = model_typemap;
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();

    // Allocating memory for probes
    char const * const output_file =
        is_spike_driven ? "spike-driven-test" : "needy-test";
    probes_firing_init(5000, "output", output_file, true);

    // Running simulation
    tw_run();
    // Simulation ends when the function exits

    // Deallocating/deinitializing everything
    probes_firing_deinit();

    tw_end();

    free(spikes[0]);

    return 0;
}

#include <ross.h>
#include "driver/neuron.h"
#include "message.h"
#include "storable_spikes.h"

static int32_t identity_map(size_t gid) {
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
    {   .init     = (init_f)    driver_neuron_init,
        .pre_run  = (pre_run_f) driver_neuron_pre_run_needy,
        .event    = (event_f)   driver_neuron_event_needy,
        .revent   = (revent_f)  driver_neuron_event_reverse_needy,
        .commit   = (commit_f)  driver_neuron_event_commit,
        .final    = (final_f)   driver_neuron_final,
        .map      = (map_f)     linear_map,
        .state_sz = sizeof(struct NeuronLP)},
    {0},
};


struct DummyNeuron {
    int8_t potential;
};


static void dummy_leak(struct DummyNeuron * dn, double d_time) {
    (void) dn;
    (void) d_time;
}


static void dummy_integrate(struct DummyNeuron * dn, float intensity) {
    (void) intensity;
    dn->potential ++;
}


static struct NeuronFiring dummy_fire(struct DummyNeuron * dn) {
    return (struct NeuronFiring) {dn->potential > 0, 1.0};
}


// Dummy store and restore are never really called. For that Optimistic mode
// should be activated. For a single PE, the only mode is Sequential.
// Nonetheless, this is the structure and their implementation (if they were to
// be ever called for reversed computation to be performed)
static_assert(sizeof(int8_t) <= MESSAGE_SIZE_REVERSE,
        "There is no space in Message to store the state of the dummy "
        "neuron. Make sure MESSAGE_SIZE_REVERSE is more than 0.");
static void dummy_store(struct DummyNeuron * dn, int8_t * storage) {
    *storage = dn->potential;
}

static void dummy_restore(struct DummyNeuron * dn, int8_t * storage) {
    dn->potential = *storage;
}


int main(int argc, char *argv[]) {
    //tw_opt_add(model_opts);
    tw_init(&argc, &argv);

    // Do some error checking?
    if (tw_nnodes() != 1) {
        fprintf(stderr, "This can only be run in one PE unfortunatelly :(."
                " Mapping is not properly defined!\n");
    }

    // Neurons settings. Hardcoded by hand

    struct DummyNeuron *neurons[5] = {
        &(struct DummyNeuron) {  // Neuron 0
            .potential = 0,
        },
        &(struct DummyNeuron) {  // Neuron 1
            .potential = 0,
        },
    };

    struct SynapseCollection synapses[5] = {
        { .num = 2,  // Neuron 0
          .synapses = (struct Synapse[2]) {
              { .gid_to_send = 0,
#ifndef NDEBUG
                .doryta_id_to_send = 0,
#endif
                .weight = 1,
                .delay = 1
              },
              { .gid_to_send = 1,
#ifndef NDEBUG
                .doryta_id_to_send = 1,
#endif
                .weight = 1,
                .delay = 1
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
                .time = 0.5,
                .intensity = 1
            },
            {0}
        },
        NULL,
    };

    // Setting the driver configuration should be done before running anything
    struct SettingsNeuronLP settings_neuron_lp = {
      .num_neurons      = 2,
      .num_neurons_pe   = 2,
      .neurons          = (void**) &neurons,
      .synapses         = synapses,
      .spikes           = spikes,
      .beat             = 1.0/16,
      .neuron_leak      = (neuron_leak_f) dummy_leak,
      .neuron_integrate = (neuron_integrate_f) dummy_integrate,
      .neuron_fire      = (neuron_fire_f) dummy_fire,
      .store_neuron         = (neuron_state_op_f) dummy_store,
      .reverse_store_neuron = (neuron_state_op_f) dummy_restore,
      .print_neuron_struct  = NULL,
      .gid_to_doryta_id = identity_map,
      .probe_events     = NULL,
    };

    driver_neuron_config(&settings_neuron_lp);

    // Setting up ROSS variables
    int const num_lps_in_pe = 2;
    tw_define_lps(num_lps_in_pe, sizeof(struct Message));
    // set the global variable and initialize each LP's type
    g_tw_lp_types = doryta_lps;
    tw_lp_setup_types();
    // note that g_tw_nlp gets set here by tw_define_lps

    // Running simulation
    tw_run();

    tw_end();

    return 0;
}

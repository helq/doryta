#ifndef DORYTA_DRIVER_NEURON_LP_H
#define DORYTA_DRIVER_NEURON_LP_H

/** @file
 * Functions implementing a neuron as a discrete event simulator
 */

#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "../message.h"

// There is no need to import ROSS headers just to define those structs
struct tw_bf;
struct tw_lp;
struct StorableSpike;

/**
 * `gid_to_send` is not the neuron to which a spike is sent but rather the LP which will
 * receive the spike. A spike can be processed by a neuron or a SynapseLP (these are in
 * charge of sending spikes in small batches to not obstruct the system)
 */
struct Synapse {
    uint64_t gid_to_send;
    uint64_t doryta_id_to_send;
    float weight;
};

/**
 * Beware: make sure `num` is correctly initialized!
 */
struct SynapseCollection {
    size_t num;
    struct Synapse * synapses; /**< An array containing all connections to other */
};

/**
 * Invariants:
 * - `neuron_struct` cannot be null
 * - `to_contact` has to be valid (`num` == 0 iff `synapses` == NULL)
 */
struct NeuronLP {
    size_t doryta_id; // This might not be the same as the GID for the neuron (it is defined as dorytaID because that is how it is caled in src/layout, but it might be anything the user wants)
    void *neuron_struct; /**< A pointer to the neuron state */
    struct SynapseCollection to_contact;
};

static inline void initialize_NeuronLP(struct NeuronLP * neuronLP) {
    neuronLP->neuron_struct = NULL;
    neuronLP->to_contact = (struct SynapseCollection){0, NULL};
}

static inline bool is_valid_NeuronLP(struct NeuronLP * neuronLP) {
    bool const struct_notnull = neuronLP->neuron_struct != NULL;
    bool const synapse_collection =
        (neuronLP->to_contact.num == 0)
        == (neuronLP->to_contact.synapses == NULL);
    return struct_notnull && synapse_collection;
}

static inline void assert_valid_NeuronLP(struct NeuronLP * neuronLP) {
#ifndef NDEBUG
    assert(neuronLP->neuron_struct != NULL);
    assert((neuronLP->to_contact.num == 0)
            == (neuronLP->to_contact.synapses == NULL));
#endif // NDEBUG
}


typedef void (*neuron_leak_f)      (void *, float);
typedef void (*neuron_integrate_f) (void *, float);
typedef bool (*neuron_fire_f)      (void *);
typedef void (*probe_event_f)      (struct NeuronLP *, struct Message *, struct tw_lp *);
typedef size_t (*id_to_id)         (size_t);
typedef void (*print_neuron_f)     (void *);
typedef void (*neuron_state_op_f)  (void *, char[MESSAGE_SIZE_REVERSE]);


/**
 * General settings for all neurons in the simulation.
 * It is assumed that `neurons` and `synapses` contain a correct
 * initialization of the given neuron type, and that they match with the LPs'
 * local IDs. What this means is that `neurons[0]` is assigned to LP with
 * local ID 0. The same happens with `synapses[0]`
 *
 * Invariants:
 * - `num_neurons_pe` > 0
 * - `num_neurons` > 0
 * - `num_neurons_pe` <= `num_neurons`
 * - `sizeof_neurons` cannot be zero
 * - `neurons` has the same number of elements as `num_neurons indicates`
 * - `neurons` and `neuron_init` cannot be both null at the same time
 * - `neuron_leak`, `neuron_integrate` and `neuron_fire` cannot be null
 * - all elements inside `neurons` must be non-null
 * - `beat` is a positive number
 * - `firing_delay` must be positive (spikes cannot be sent to the past,
 *   but can be issued almost instantly). The smallest delay possible is less
 *   than one `beat` but never instanteneous
 *
 * Possible future invariants:
 * - `beat` should be a power of 2
 * - If `spikes` is not null, it points to `num_neurons` spikes (array of pointers ending in a null element)
 */
struct SettingsNeuronLP {
    size_t                     num_neurons;
    size_t                     num_neurons_pe;
    // Yes, this is not ideal, best would be to have everything contained
    // within the neuron, but ROSS has no separation for memory allocation
    // and execution. (Both are performed at `tw_run`.)
    void                    ** neurons;
    struct SynapseCollection * synapses;
    struct StorableSpike    ** spikes;
    double                     beat; //<! Heartbeat frequency
    int                        firing_delay;
    neuron_leak_f              neuron_leak;
    neuron_integrate_f         neuron_integrate;
    neuron_fire_f              neuron_fire;
    neuron_state_op_f          store_neuron;
    neuron_state_op_f          reverse_store_neuron;
    print_neuron_f             print_neuron_struct;
    id_to_id                   gid_to_doryta_id; //<! Getting neuron ID (aka, DorytaID)
    probe_event_f            * probe_events; //<! A list of functions to call to record/trace the computation
};

static inline bool is_valid_SettingsPE(struct SettingsNeuronLP * settingsPE) {
    bool const basic_non_nullness =
           settingsPE->neurons != NULL
        && settingsPE->neuron_leak != NULL
        && settingsPE->neuron_integrate != NULL
        && settingsPE->neuron_fire != NULL
        && settingsPE->gid_to_doryta_id != NULL
        && settingsPE->store_neuron != NULL
        && settingsPE->reverse_store_neuron != NULL;
    bool const correct_neuron_sizes =
           settingsPE->num_neurons > 0
        && settingsPE->num_neurons_pe > 0
        && settingsPE->num_neurons_pe <= settingsPE->num_neurons;
    bool const beat_validity = settingsPE->beat > 0
                            && !isnan(settingsPE->beat)
                            && !isinf(settingsPE->beat);
    bool const firing_delay_validity = 1 <= settingsPE->firing_delay;
    if (!(basic_non_nullness && correct_neuron_sizes
          && beat_validity && firing_delay_validity)) {
        return false;
    }
    for (size_t i = 0; i < settingsPE->num_neurons_pe; i++) {
        if (settingsPE->neurons[i] == NULL) {
            return false;
        }
    }
    return true;
}

static inline void assert_valid_SettingsPE(struct SettingsNeuronLP * settingsPE) {
#ifndef NDEBUG
    assert(settingsPE->num_neurons_pe > 0);
    assert(settingsPE->neurons != NULL);
    assert(settingsPE->neuron_leak != NULL);
    assert(settingsPE->neuron_integrate != NULL);
    assert(settingsPE->neuron_fire != NULL);
    assert(settingsPE->gid_to_doryta_id != NULL);
    assert(settingsPE->store_neuron != NULL);
    assert(settingsPE->reverse_store_neuron != NULL);
    assert(settingsPE->beat > 0);
    assert(!isnan(settingsPE->beat));
    assert(!isinf(settingsPE->beat));
    assert(1 <= settingsPE->firing_delay);
    for (size_t i = 0; i < settingsPE->num_neurons_pe; i++) {
        assert(settingsPE->neurons[i] != NULL);
    }
#endif // NDEBUG
}

/** Setting global variables for the simulation. */
void neuronLP_config(struct SettingsNeuronLP *);

/** Neuron initialization. */
void neuronLP_init(struct NeuronLP *neuronLP, struct tw_lp *lp);

/** Forward event handler. */
void neuronLP_event(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Reverse event handler. */
void neuronLP_event_reverse(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Commit event handler. */
void neuronLP_event_commit(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Cleaning and printing info before shut down. */
void neuronLP_final(struct NeuronLP *neuronLP, struct tw_lp *lp);

#endif /* end of include guard */
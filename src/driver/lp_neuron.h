#ifndef DORYTA_DRIVER_LP_NEURON_H
#define DORYTA_DRIVER_LP_NEURON_H

/** @file
 * Functions implementing a neuron as a discrete event simulator
 */

#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>

// There is no need to import ROSS headers just to define those structs
struct tw_bf;
struct tw_lp;
struct Message;
struct StorableSpike;

/**
 * `id_to_send` is not the neuron to which a spike is sent but rather the LP which will
 * receive the spike. A spike can be processed by a neuron or a SynapseLP (these are in
 * charge of sending spikes in small batches to not obstruct the system)
 */
struct Synapse {
    uint64_t id_to_send;
    float weight;
};

struct SynapseCollection {
    size_t num;
    struct Synapse * synapses; /**< An array containing all connections to other */
};

/**
 * Invariants:
 * - `neuron_struct` cannot be null
 * - `last_fired` can only be -1 or non-negative number
 * - `next_heartbeat` can only be -1 or non-negative number
 * - `last_fired <= next_heartbeat` can only be -1 or non-negative number
 */
struct NeuronLP {
    void *neuron_struct; /**< A pointer to the neuron state */
    struct SynapseCollection to_contact;
    bool sent_fire_msg;
    double last_fired;
    double next_heartbeat;
};

static inline void initialize_NeuronLP(struct NeuronLP * neuronLP) {
    neuronLP->to_contact = (struct SynapseCollection){0, NULL};
    neuronLP->sent_fire_msg = false;
    neuronLP->last_fired = -1;
    neuronLP->next_heartbeat = -1;
}

static inline bool is_valid_NeuronLP(struct NeuronLP * neuronLP) {
    bool const struct_notnull = neuronLP->neuron_struct != NULL;
    bool const last_fired_correct =
        (neuronLP->last_fired == -1 || neuronLP->last_fired >= 0)
        && !isnan(neuronLP->last_fired)
        && !isinf(neuronLP->last_fired);
    bool const next_heartbeat_correct =
        (neuronLP->next_heartbeat == -1 || neuronLP->next_heartbeat >= 0)
        && !isnan(neuronLP->next_heartbeat)
        && !isinf(neuronLP->next_heartbeat);
    bool const fired_less_than_heartbeat =
        neuronLP->last_fired <= neuronLP->next_heartbeat;
    return struct_notnull
        && last_fired_correct && next_heartbeat_correct
        && fired_less_than_heartbeat;
}

static inline void assert_valid_NeuronLP(struct NeuronLP * neuronLP) {
#ifndef NDEBUG
    assert(neuronLP->neuron_struct != NULL);
    assert(neuronLP->last_fired == -1 || neuronLP->last_fired >= 0);
    assert(!isnan(neuronLP->last_fired));
    assert(!isinf(neuronLP->last_fired));
    assert(neuronLP->next_heartbeat == -1 || neuronLP->next_heartbeat >= 0);
    assert(!isnan(neuronLP->next_heartbeat));
    assert(!isinf(neuronLP->next_heartbeat));
    assert(neuronLP->last_fired <= neuronLP->next_heartbeat);
#endif // NDEBUG
}

typedef void (*neuron_leak_f)      (void *);
typedef void (*neuron_integrate_f) (void *, float);
typedef bool (*neuron_fire_f)      (void *);
typedef void (*probe_event_f)      (struct NeuronLP *, struct Message *, struct tw_lp *);

/**
 * If `spikes` is NULL
 * Invariants:
 * - `num_neurons` > 0
 * - `neurons` cannot be null (and has the same number of elements as `num_neurons indicates`)
 * - `neuron_leaf`, `neuron_integrate` and `neuron_fire` cannot be null
 * - all elements inside `neurons` must be non-null
 * - `beat` is a positive number
 * - `firing_delay` cannot be smaller than `beat`
 *
 * Possible future invariants:
 * - `beat` should be a power of 2
 * - If `spikes` is not null, it points to `num_neurons` spikes (array of pointers ending in a null element)
 */
struct SettingsPE {
    size_t                     num_neurons;
    void                    ** neurons; // Yes, this is redundant but only slightly because we are copying a small struct (NeuronLP)
    struct SynapseCollection * synapses;
    struct StorableSpike    ** spikes;
    double                     beat; //<! Heartbeat frequency
    double                     firing_delay; // Only to be used in delayed mode
    neuron_leak_f              neuron_leak;
    neuron_integrate_f         neuron_integrate;
    neuron_fire_f              neuron_fire;
    probe_event_f            * probe_events; //<! A list of functions to call to record/trace the computation
};

static inline bool is_valid_SettingsPE(struct SettingsPE * settingsPE) {
    bool basic_non_nullness =
        settingsPE->num_neurons > 0
        && settingsPE->neurons != NULL
        && settingsPE->neuron_leak != NULL
        && settingsPE->neuron_integrate != NULL
        && settingsPE->neuron_fire != NULL;
    bool beat_validity = settingsPE->beat > 0
                      && !isnan(settingsPE->beat)
                      && !isinf(settingsPE->beat);
    bool firing_delay_validity =
           settingsPE->beat <= settingsPE->firing_delay
        && !isnan(settingsPE->firing_delay)
        && !isinf(settingsPE->firing_delay);
    if (!(basic_non_nullness && beat_validity && firing_delay_validity)) {
        return false;
    }
    for (size_t i = 0; i < settingsPE->num_neurons; i++) {
        if (settingsPE->neurons[i] == NULL) {
            return false;
        }
    }
    return true;
}

static inline void assert_valid_SettingsPE(struct SettingsPE * settingsPE) {
#ifndef NDEBUG
    assert(settingsPE->num_neurons > 0);
    assert(settingsPE->neurons != NULL);
    assert(settingsPE->neuron_leak != NULL);
    assert(settingsPE->neuron_integrate != NULL);
    assert(settingsPE->neuron_fire != NULL);
    assert(settingsPE->beat > 0);
    assert(!isnan(settingsPE->beat));
    assert(!isinf(settingsPE->beat));
    assert(settingsPE->beat <= settingsPE->firing_delay);
    assert(!isnan(settingsPE->firing_delay));
    assert(!isinf(settingsPE->firing_delay));
    for (size_t i = 0; i < settingsPE->num_neurons; i++) {
        assert(settingsPE->neurons[i] != NULL);
    }
#endif // NDEBUG
}

/** Setting global variables for the simulation. */
void neuron_pe_config(struct SettingsPE *);

/** Neuron initialization. */
void neuronLP_init(struct NeuronLP *neuronLP, struct tw_lp *lp);

/** Forward event handler. */
void neuronLP_event(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Reverse event handler. */
//void neuronLP_event_reverse(
//        struct NeuronLP *neuronLP,
//        struct tw_bf *bit_field,
//        struct Message *message,
//        struct tw_lp *lp);

/** Commit event handler. */
void neuronLP_event_commit(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Cleaning and printing info before shut down. */
//void neuronLP_final(struct NeuronLP *neuronLP, struct tw_lp *lp);

#endif /* end of include guard */

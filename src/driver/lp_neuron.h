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

struct Synapse {
    uint64_t neuron_id;
    float weight;
};

struct SynapseCollection {
    size_t num;
    struct Synapse * synapses; /**< An array containing all connections to other */
};

/**
 * Invariants:
 * - `neuron_struct` cannot be null
 */
struct NeuronLP {
    void *neuron_struct; /**< A pointer to the neuron state */
    struct SynapseCollection to_contact;
    bool sent_heartbeat;
};

static inline bool is_valid_NeuronLP(struct NeuronLP * neuronLP) {
    return neuronLP->neuron_struct != NULL;
}

static inline void assert_valid_NeuronLP(struct NeuronLP * neuronLP) {
#ifndef NDEBUG
    assert(neuronLP->neuron_struct != NULL);
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
    if (!(basic_non_nullness && beat_validity)) {
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

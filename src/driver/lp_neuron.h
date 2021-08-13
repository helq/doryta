#ifndef DORYTA_DRIVER_LP_NEURON_H
#define DORYTA_DRIVER_LP_NEURON_H

/** @file
 * Functions implementing a neuron as a discrete event simulator
 */

#include <stdbool.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>

// There is no need to import ROSS headers just to define those structs
struct tw_bf;
struct tw_lp;
struct Message;

struct Synapse {
    uint64_t neuron_id;
    float weight;
};

struct NeuronLP {
    void *neuron_struct; /**< A pointer to the neuron state */
    struct {
        size_t num_neurons_to_connect;
        struct Synapse * other_neurons; /**< An array containing all connections to other */
    };
};

static inline bool is_valid_NeuronLP(struct NeuronLP * neuronLP) {
    return false;
}

static inline void assert_valid_NeuronLP(struct NeuronLP * neuronLP) {
#ifndef NDEBUG
    assert(false);
#endif // NDEBUG
}

typedef void (*neuron_leak_f)      (void *);
typedef void (*neuron_integrate_f) (void *, float);
typedef bool (*neuron_fire_f)      (void *);
typedef void (*probe_event_f)      (struct NeuronLP *, struct Message *, struct tw_lp *);

struct SettingsPE {
    size_t num_neurons;
    void **neurons;
    neuron_leak_f       neuron_leak;
    neuron_integrate_f  neuron_integrate;
    neuron_fire_f       neuron_fire;
    probe_event_f     * probe_events; //<! A list of functions to call to record/trace the computation
};

/** Setting global variables for the simulation. */
void neuron_pe_config(struct SettingsPE);

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

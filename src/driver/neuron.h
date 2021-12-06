#ifndef DORYTA_DRIVER_NEURON_H
#define DORYTA_DRIVER_NEURON_H

/** @file
 * Functions implementing a neuron as a discrete event simulator
 */

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
    int32_t doryta_id_to_send;
    float weight;
};

/**
 * Beware: make sure `num` is correctly initialized!
 */
struct SynapseCollection {
    int32_t num;
    struct Synapse * synapses; /**< An array containing all connections to other */
};

/**
 * Invariants:
 * - `neuron_struct` cannot be null
 * - `to_contact` has to be valid (`num` == 0 iff `synapses` == NULL)
 * - `last_heartbeat` is never negative
 */
struct NeuronLP {
    int32_t doryta_id; // This might not be the same as the GID for the neuron (it is defined as dorytaID because that is how it is caled in src/layout, but it might be anything the user wants)
    void *neuron_struct; /**< A pointer to the neuron state */
    struct SynapseCollection to_contact;

    // spike-driven mode only parameters
    struct {
        double last_heartbeat;
        bool next_heartbeat_sent;
    };
};

static inline void initialize_NeuronLP(struct NeuronLP * neuronLP) {
    neuronLP->neuron_struct = NULL;
    neuronLP->to_contact = (struct SynapseCollection){0, NULL};
    neuronLP->last_heartbeat = 0;
    neuronLP->next_heartbeat_sent = false;
}

static inline bool is_valid_NeuronLP(struct NeuronLP * neuronLP) {
    bool const struct_notnull = neuronLP->neuron_struct != NULL;
    bool const synapse_collection =
        (neuronLP->to_contact.num == 0)
        == (neuronLP->to_contact.synapses == NULL);
    bool const positive_heartbeat = neuronLP->last_heartbeat >= 0;
    return struct_notnull && synapse_collection && positive_heartbeat;
}

static inline void assert_valid_NeuronLP(struct NeuronLP * neuronLP) {
#ifndef NDEBUG
    assert(neuronLP->neuron_struct != NULL);
    assert((neuronLP->to_contact.num == 0)
            == (neuronLP->to_contact.synapses == NULL));
    assert(neuronLP->last_heartbeat >= 0);
#endif // NDEBUG
}


typedef void (*neuron_leak_f)      (void *, double);
typedef void (*neuron_leak_big_f)  (void *, double, double);
typedef void (*neuron_integrate_f) (void *, float);
typedef bool (*neuron_fire_f)      (void *);
typedef void (*probe_event_f)      (struct NeuronLP *, struct Message *, struct tw_lp *);
typedef int32_t (*id_to_dorytaid)  (size_t);
typedef void (*print_neuron_f)     (void *);
typedef void (*neuron_state_op_f)  (void *, char[MESSAGE_SIZE_REVERSE]);


/**
 * General settings for all neurons in the simulation.
 * It is assumed that `neurons` and `synapses` contain a correct
 * initialization of the given neuron type, and that they match with the LPs'
 * local IDs. What this means is that `neurons[0]` is assigned to LP with
 * local ID 0. The same happens with `synapses[0]`.
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
    /** Total number of neurons across all PEs. */
    int                     num_neurons;
    /** Total number of neurons on this PEs. */
    int                     num_neurons_pe;
    // Yes, this is not ideal, best would be to have everything contained
    // within the neuron, but ROSS has no separation for memory allocation
    // and execution. (Both are performed at `tw_run`.)
    /** Pointers to neurons of the given type. One pointer per neuron, for a
     * total of `num_neurons_pe`. No neuron can be NULL. */
    void                    ** neurons;
    /** Outgoing synapses for each neuron in this PE. As with `neurons`, this
     * must be an array of exactly `num_neurons_pe` elements, or the program
     * might segfault. */
    struct SynapseCollection * synapses;
    /** Input spikes for each neuron in PE. The first pointer corresponds to an
     * array to arrays. The second pointer corresponds to the array of spikes
     * for a specific neuron. The array of spikes must finalize in zero. An
     * `intensity` of zero indicates the end of the array. The pointer of
     * pointers can be NULL, and individual arrays of spikes (per neuron) can
     * be NULL. */
    struct StorableSpike    ** spikes;
    /** Heartbeat frequency. A positive number, hopefully small enough to
     * simulate the real-valued behaviour of neurons in continuous time. It's a
     * simulation, thus it is discretized. The value of the heartbeat should be
     * a power of 2. There is no warranty that the execution will be
     * deterministic if the heartbeat is not a power of 2. */
    double                     beat;
    /** Indicates how many "heartbeat" timestepts to wait since a spike that
     * triggers firing is received. The smallest value is 1. */
    int                        firing_delay;
    /** Performs the _leak_ operation on a neuron with a delta step of `dt` (a
     * heartbeat lenght). */
    neuron_leak_f              neuron_leak;
    /** Performs the _leak_ operation on a neuron, assuming no input current
     * (no spikes), over an arbitrary period of time. A third parameter is
     * given, the heartbeat, in case the equation describing the neuron cannot
     * be analytically solved and must be approximated by a simulation (a
     * loop). This function is only called on _spike driven_ mode. */
    neuron_leak_big_f          neuron_leak_bigdt;
    /** Performs the _integrate_ operation on a neuron. This often equates to a
     * single addition operation. */
    neuron_integrate_f         neuron_integrate;
    /** Performs the _fire_ operation. This determines whether the condition
     * for firing in the neuron has been met, changes the state of the neuron
     * to its state after firing and returns the true if it fired. */
    neuron_fire_f              neuron_fire;
    /** This operation is given the neuron state and a pointer to a reserved
     * space of size `MESSAGE_SIZE_REVERSE`. The operation must save the full
     * (modifiable) state of the neuron into the reserved space. */
    neuron_state_op_f          store_neuron;
    /** This operation is the inverse of `store_neuron`. It must modify the
     * state of the neuron given the data stored. */
    neuron_state_op_f          reverse_store_neuron;
    /** An optional function in charge of printing in one line the state of the
     * neuron at the end of the simulation. Use only for debug purposes as the
     * output get clogged with large models with many neurons. */
    print_neuron_f             print_neuron_struct;
    /** This function takes a GID and produces a neuron ID (aka, DorytaID). */
    id_to_dorytaid             gid_to_doryta_id;
    /** A list of functions to call to record/trace the computation. It can be
     * NULL. The array must be NULL terminated. */
    probe_event_f            * probe_events;
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
    for (int i = 0; i < settingsPE->num_neurons_pe; i++) {
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
    for (int i = 0; i < settingsPE->num_neurons_pe; i++) {
        assert(settingsPE->neurons[i] != NULL);
    }
#endif // NDEBUG
}

/** Setting global variables for the simulation. */
void driver_neuron_config(struct SettingsNeuronLP *);

/** Neuron initialization. */
void driver_neuron_init(struct NeuronLP *neuronLP, struct tw_lp *lp);

/** Neuron pre-run handler (only to be used by needy mode). */
void driver_neuron_pre_run_needy(struct NeuronLP *neuronLP, struct tw_lp *lp);

/** Forward event handler for needy mode. */
void driver_neuron_event_needy(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Forward event handler for spike-driven mode. Only works if
 * `neuron_leak_bigdt` is defined. */
void driver_neuron_event_spike_driven(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Reverse event handler for needy mode. */
void driver_neuron_event_reverse_needy(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Reverse event handler. */
void driver_neuron_event_reverse_spike_driven(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Commit event handler. */
void driver_neuron_event_commit(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *message,
        struct tw_lp *lp);

/** Cleaning and printing info before shut down. */
void driver_neuron_final(struct NeuronLP *neuronLP, struct tw_lp *lp);

#endif /* end of include guard */

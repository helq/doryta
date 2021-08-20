#include "lp_neuron.h"
#include "../message.h"
#include "../storable_spikes.h"
#include <stdbool.h>
#include <ross.h>

// If spikes and heartbeats "occur" at the same time (ie, they are scheduled
// for the same timestamp), then all spike events will be processed before any
// heartbeat spike does
#define SPIKE_PRIORITY 0.5
#define HEARTBEAT_PRIORITY 0.8

struct SettingsPE settings = {0};
bool settings_initialized = false;
double firing_delay_double = -1;


void neuron_pe_config(struct SettingsPE * settings_in) {
    assert_valid_SettingsPE(settings_in);
    settings = *settings_in;
    settings_initialized = true;
    firing_delay_double = (settings.firing_delay - 0.5) * settings.firing_delay;

    // TODO: This PE should communicate with all the others to see if the total
    // number of neurons is what is supposed to be
}


static inline void send_heartbeat(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    (void) neuronLP;
    uint64_t const self = lp->gid;
    struct tw_event * const event
        = tw_event_new_user_prio(self, settings.beat, lp, HEARTBEAT_PRIORITY);
    // Updating when next heartbeat will occur
    struct Message * const msg = tw_event_data(event);
    initialize_Message(msg, MESSAGE_TYPE_heartbeat);
    assert_valid_Message(msg);
    tw_event_send(event);
}


static inline void send_spike(
        struct NeuronLP *neuronLP, struct tw_lp *lp, double diff) {
    if (neuronLP->to_contact.num > 0) {
        for (size_t i = 0; i < neuronLP->to_contact.num; i++) {
            struct Synapse const synap =
                neuronLP->to_contact.synapses[i];

            struct tw_event * const event =
                tw_event_new_user_prio(synap.id_to_send, diff, lp, SPIKE_PRIORITY);
            struct Message * const msg = tw_event_data(event);
            initialize_Message(msg, MESSAGE_TYPE_spike);
            msg->current = synap.weight;
            assert_valid_Message(msg);
            tw_event_send(event);
        }
    }
}


static inline void send_spike_from_StorableSpike(
        struct NeuronLP *neuronLP,
        struct tw_lp *lp,
        struct StorableSpike * spike) {
    // A StorableSpike is only to be sent and processed by the same neuron that
    // it's indicated in the StorableSpike
    assert(neuronLP->id == spike->neuron);

    uint64_t const self = lp->gid;
    struct tw_event * const event
        = tw_event_new_user_prio(self, spike->time, lp, SPIKE_PRIORITY);
    struct Message * const msg = tw_event_data(event);
    initialize_Message(msg, MESSAGE_TYPE_spike);
    msg->current = spike->intensity;
    assert_valid_Message(msg);
    tw_event_send(event);
}


// LP initialization. Called once for each LP
void neuronLP_init(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    assert(settings_initialized);

    // TODO: This should be changed to neuron id in current PE
    uint64_t const neuron_id = settings.get_neuron_local_pos_init(lp);
    assert(neuron_id < settings.num_neurons_pe);

    // Initializing NeuronLP from parameters defined by the
    initialize_NeuronLP(neuronLP);
    neuronLP->id = settings.get_neuron_gid(lp);
    neuronLP->neuron_struct = settings.neurons[neuron_id];
    if (settings.synapses != NULL) {
        neuronLP->to_contact = settings.synapses[neuron_id];
    }

    // Creating spike events
    if (settings.spikes != NULL && settings.spikes[neuron_id] != NULL) {
        struct StorableSpike * spikes_for_neuron = settings.spikes[neuron_id];
        // spikes is a pointer to an array of NULL/zero terminated spikes
        while(spikes_for_neuron->intensity != 0) {
            assert_valid_StorableSpike(spikes_for_neuron);
            send_spike_from_StorableSpike(neuronLP, lp, spikes_for_neuron);
            spikes_for_neuron++;
        }
    }

    // send initial heartbeat
    send_heartbeat(neuronLP, lp);
    assert_valid_NeuronLP(neuronLP);
}


// Forward event handler
/**
 * `MESSAGE_TYPE_fire` are (almost) guaranteed to not be executed at the same time as a
 * `MESSAGE_TYPE_heartbeat`. The same cannot be said of any of the other relations between
 * events. It is possible that a heartbeat and a spike messages are schedule to run at the
 * same instant, or a fire and a spike message. If this happens, it is pretty much
 * impossible to know what will happen. This is an intrinsic limitation of PDES
 * unfortunately. There is no way to enforce an ordering between the kinds of events.
 * Where this ever to happen, there would be no need for shifting tricks.
 */
void neuronLP_event(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    (void) bit_field;
    assert_valid_Message(msg);

    msg->time_processed = tw_now(lp);

    switch (msg->type) {
        case MESSAGE_TYPE_heartbeat: {
            // If more than one "fire msg" was sent
            bool const fired = settings.neuron_fire(neuronLP->neuron_struct);
            if (fired) {
                send_spike(neuronLP, lp, firing_delay_double);
                msg->fired = true;
            }
            settings.neuron_leak(neuronLP->neuron_struct);
            send_heartbeat(neuronLP, lp);
            break;
        }

        case MESSAGE_TYPE_spike:
            settings.neuron_integrate(neuronLP->neuron_struct, msg->current);
            break;
    }
}


// Reverse Event Handler
// Notice that all operations are reversed using the data stored in either the reverse
// message or the bit field
//void neuronLP_event_reverse(
//        struct NeuronLP *neuronLP,
//        struct tw_bf *bit_field,
//        struct Message *msg,
//        struct tw_lp *lp) {
//}


// Commit event handler
// This function is only called when it can be make sure that the message won't be
// roll back. Either the commit or reverse handler will be called, not both
void neuronLP_event_commit(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    (void) bit_field;
    if (settings.probe_events != NULL) {
        for (size_t i = 0; settings.probe_events[i] != NULL; i++) {
            probe_event_f record_func = settings.probe_events[i];
            record_func(neuronLP, msg, lp);
        }
    }
}


// The finalization function
// Reporting any final statistics for this LP in the file previously opened
//void neuronLP_final(struct NeuronLP *neuronLP, struct tw_lp *lp) {
//}

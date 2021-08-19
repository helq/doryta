#include "lp_neuron.h"
#include "../message.h"
#include "../mapping.h"
#include "../storable_spikes.h"
#include <stdbool.h>
#include <ross.h>

struct SettingsPE settings = {0};
bool settings_initialized = false;


void neuron_pe_config(struct SettingsPE * settings_in) {
    assert_valid_SettingsPE(settings_in);
    settings = *settings_in;
    settings_initialized = true;
}


static void send_heartbeat(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    uint64_t const self = lp->gid;
    struct tw_event * const event = tw_event_new(self, settings.beat, lp);
    // Updating when next heartbeat will occur
    neuronLP->next_heartbeat = event->recv_ts;
    struct Message * const msg = tw_event_data(event);
    initialize_Message(msg);
    msg->type = MESSAGE_TYPE_heartbeat;
    assert_valid_Message(msg);
    tw_event_send(event);
}


static void send_spike(struct NeuronLP *neuronLP, struct tw_lp *lp, double diff) {
    if (neuronLP->to_contact.num > 0) {
        for (size_t i = 0; i < neuronLP->to_contact.num; i++) {
            struct Synapse const synap =
                neuronLP->to_contact.synapses[i];

            struct tw_event * const event =
                tw_event_new(synap.id_to_send, diff, lp);
            struct Message * const msg = tw_event_data(event);
            initialize_Message(msg);
            msg->type = MESSAGE_TYPE_spike;
            msg->current = synap.weight;
            assert_valid_Message(msg);
            tw_event_send(event);
        }
    }
}


static void send_spike_at_shift(struct tw_lp *lp, struct StorableSpike * spike) {
    assert(get_neuron_id(lp) == spike->neuron);

    uint64_t const self = lp->gid;
    struct tw_event * const event = tw_event_new(self, spike->time, lp);
    struct Message * const msg = tw_event_data(event);
    initialize_Message(msg);
    msg->type = MESSAGE_TYPE_spike;
    msg->current = spike->intensity;
    assert_valid_Message(msg);
    tw_event_send(event);
}


static void send_fire_msg(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    if (neuronLP->sent_fire_msg == false) {
        uint64_t const self = lp->gid;
        double next_heartbeat = neuronLP->next_heartbeat;
        double const now = tw_now(lp);
        double shift = 0.0;

        // We hope the heartbeat won't break much havoc as it coincides with
        // the spike that we are currently processing. To prevent future chaos,
        // with even more spikes coinciding with the heartbeat, we shift a tiny
        // bit into the future when the spike was supposedly processed (which
        // is no longer now). This only happens because `firing_delay` can
        // be equal to `beat` :S
        if (next_heartbeat == tw_now(lp)) {
            next_heartbeat += settings.beat;
            shift = settings.beat == settings.firing_delay ? settings.beat / 4096: 0;
        }

        // Finding the precise moment to create the event (just a tiny bit
        // before the heartbeat)
        double const almost_at_next_beat =
            nextafter(next_heartbeat, now) - now;

        // Constructing firing event
        struct tw_event * event = tw_event_new(self, almost_at_next_beat, lp);
        assert(event->recv_ts < next_heartbeat);
        struct Message * const msg = tw_event_data(event);
        initialize_Message(msg);
        msg->type = MESSAGE_TYPE_fire;
        msg->spike_time_arrival = tw_now(lp) + shift;
        assert_valid_Message(msg);
        tw_event_send(event);

        neuronLP->sent_fire_msg = true;
    }
}


// LP initialization. Called once for each LP
void neuronLP_init(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    assert(settings_initialized);

    // TODO: This should be changed to neuron id in current PE
    uint64_t const neuron_id = get_neuron_id(lp);
    assert(neuron_id < settings.num_neurons);
    initialize_NeuronLP(neuronLP);
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
            send_spike_at_shift(lp, spikes_for_neuron);
            spikes_for_neuron++;
        }
    }

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
        case MESSAGE_TYPE_heartbeat:
            settings.neuron_leak(neuronLP->neuron_struct);
            send_heartbeat(neuronLP, lp);
            break;
        case MESSAGE_TYPE_spike: {
            settings.neuron_integrate(neuronLP->neuron_struct, msg->current);
            send_fire_msg(neuronLP, lp);
            break;
        case MESSAGE_TYPE_fire:
            // If more than one "fire msg" was sent
            if (neuronLP->last_fired < tw_now(lp)) {
                bool const fired = settings.neuron_fire(neuronLP->neuron_struct);
                if (fired) {
                    double const diff_for_firing =
                        msg->spike_time_arrival + settings.firing_delay - tw_now(lp);
                    send_spike(neuronLP, lp, diff_for_firing);
                    msg->fired = true;
                    neuronLP->last_fired = tw_now(lp);
                }
            }
            neuronLP->sent_fire_msg = false;
            break;
        }
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

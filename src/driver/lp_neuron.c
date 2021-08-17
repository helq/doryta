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
    (void) neuronLP;
    (void) lp;
    if (neuronLP->sent_heartbeat) {
        return;
    }
    // TODO: So, when should be a heartbeat be scheduled? It has to
    // divide the time into "beat" portions. It only has allowed
    // to send heartbeats for those specific times
    uint64_t const self = lp->gid;
    struct tw_event *event = tw_event_new(self, settings.beat, lp);
    struct Message * const msg = tw_event_data(event);
    initialize_Message(msg);
    msg->type = MESSAGE_TYPE_heartbeat;
    assert_valid_Message(msg);
    tw_event_send(event);
    neuronLP->sent_heartbeat = true;
}


static void send_spike(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    if (neuronLP->to_contact.num > 0) {
        for (size_t i = 0; i < neuronLP->to_contact.num; i++) {
            struct Synapse const synap =
                neuronLP->to_contact.synapses[i];

            uint64_t const id_to = get_gid_for_neuronid(synap.neuron_id);

            struct tw_event * const e = tw_event_new(id_to, settings.beat/2, lp);
            struct Message * const msg = tw_event_data(e);
            initialize_Message(msg);
            msg->type = MESSAGE_TYPE_spike;
            msg->current = synap.weight;
            assert_valid_Message(msg);
            tw_event_send(e);
        }
    } else {
        //printf("FIRED!%f\n", tw_now(lp));
        // TODO: save spike firing using probe
    }
}


static void send_spike_at_shift(struct tw_lp *lp, struct StorableSpike * spike) {
    assert(get_neuron_id(lp) == spike->neuron);

    uint64_t const self = lp->gid;
    struct tw_event * const e = tw_event_new(self, spike->time, lp);
    struct Message * const msg = tw_event_data(e);
    initialize_Message(msg);
    msg->type = MESSAGE_TYPE_spike;
    msg->current = spike->intensity;
    assert_valid_Message(msg);
    tw_event_send(e);
}


// LP initialization. Called once for each LP
void neuronLP_init(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    assert(settings_initialized);

    // TODO: This should be changed to neuron id in current PE
    uint64_t const neuron_id = get_neuron_id(lp);
    assert(neuron_id < settings.num_neurons);

    neuronLP->neuron_struct = settings.neurons[neuron_id];
    neuronLP->sent_heartbeat = false;
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
            neuronLP->sent_heartbeat = false;
            settings.neuron_leak(neuronLP->neuron_struct);
            send_heartbeat(neuronLP, lp);
            break;
        case MESSAGE_TYPE_spike: {
            settings.neuron_integrate(neuronLP->neuron_struct, msg->current);
            bool const spike = settings.neuron_fire(neuronLP->neuron_struct);
            if (spike) {
                send_spike(neuronLP, lp);
            }
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
            probe_event_f record_fun = settings.probe_events[i];
            record_fun(neuronLP, msg, lp);
        }
    }
}


// The finalization function
// Reporting any final statistics for this LP in the file previously opened
//void neuronLP_final(struct NeuronLP *neuronLP, struct tw_lp *lp) {
//}

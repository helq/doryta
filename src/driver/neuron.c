#include "neuron.h"
#include "../storable_spikes.h"
#include <ross.h>

// If spikes and heartbeats "occur" at the same time (ie, they are scheduled
// for the same timestamp), then all heartbeat events will be processed before
// any spike event does.
//
// This ordering is not arbitrary:
// Processing a heartbeat involves executing leak and fire operations;
// processing spikes involves executing the integration operation.
// When loading spikes from a file, if many of them occur at the same instant,
// we would like to process them in batches before a heartbeat occurs (to
// prevent event explosion). If the timestampt of these spikes coincided with a
// heartbeat and heartbeats occurred _after_ spikes, then we could only
// schedule more spikes (in batches) for the same timestampt, otherwise only a
// batch of spikes would be processed by the heartbeat (leak and fire) and the
// rest of them would be processed by the next heartbeat. Big aggregations of
// spikes (events) can be broken down into batches of events, each to be
// processed and memory freed, either by scheduling batches for an instant
// further in the future or by scheduling batches for the same instant. The
// second option is not really feasible. It would involve abusing the
// tiebreaking mechanism and praying for GVT to work.
//
// TL;DR: to prevent event/message/spike explotion, we can schedule spikes in
// batches, all to be processed between two heartbeats. When spikes are sent
// in batches and loaded from a file/memory, it is easier to process heartbeats
// before spikes (if their timestamp coincide) because spikes can be easily
// rescheduled to an instant in further in the future (within the heartbeat
// time).
#define SPIKE_PRIORITY 0.8
#define HEARTBEAT_PRIORITY 0.5

struct SettingsNeuronLP settings = {0};
bool settings_initialized = false;
double firing_delay_double = -1;


void driver_neuron_config(struct SettingsNeuronLP * settings_in) {
    assert_valid_SettingsPE(settings_in);
    settings = *settings_in;
    settings_initialized = true;
    firing_delay_double = (settings.firing_delay - 0.5) * settings.beat;

    // TODO: This PE should communicate with all the others to see if the total
    // number of neurons is what is supposed to be (only to be run as an assert)
}


static inline void send_heartbeat_at(struct NeuronLP *neuronLP, struct tw_lp *lp, double dt) {
    (void) neuronLP;
    uint64_t const self = lp->gid;
    struct tw_event * const event
        = tw_event_new_user_prio(self, dt, lp, HEARTBEAT_PRIORITY);
    // Updating when next heartbeat will occur
    struct Message * const msg = tw_event_data(event);
    initialize_Message(msg, MESSAGE_TYPE_heartbeat);
    assert_valid_Message(msg);
    tw_event_send(event);
}


static inline void send_heartbeat(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    send_heartbeat_at(neuronLP, lp, settings.beat);
}


static inline void send_spike(
        struct NeuronLP *neuronLP, struct tw_lp *lp, double diff) {
    if (neuronLP->to_contact.num > 0) {
        for (int32_t i = 0; i < neuronLP->to_contact.num; i++) {
            struct Synapse const synap =
                neuronLP->to_contact.synapses[i];

            struct tw_event * const event =
                tw_event_new_user_prio(synap.gid_to_send, diff, lp, SPIKE_PRIORITY);
            struct Message * const msg = tw_event_data(event);
            initialize_Message(msg, MESSAGE_TYPE_spike);
            msg->neuron_from = neuronLP->doryta_id;
            msg->neuron_to = synap.doryta_id_to_send;
            msg->neuron_from_gid = lp->gid;
            msg->neuron_to_gid = synap.gid_to_send;
            msg->spike_current = synap.weight;
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
    assert(neuronLP->doryta_id == spike->neuron);

    uint64_t const self = lp->gid;
    struct tw_event * const event
        = tw_event_new_user_prio(self, spike->time, lp, SPIKE_PRIORITY);
    struct Message * const msg = tw_event_data(event);
    initialize_Message(msg, MESSAGE_TYPE_spike);
    msg->neuron_from = neuronLP->doryta_id;
    msg->neuron_to = neuronLP->doryta_id;
    msg->neuron_from_gid = lp->id;
    msg->neuron_to_gid = self;
    msg->spike_current = spike->intensity;
    assert_valid_Message(msg);
    tw_event_send(event);
}


// LP initialization. Called once for each LP
void driver_neuron_init(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    assert(settings_initialized);

    uint64_t const local_id = lp->id;
    assert(local_id < (uint64_t) settings.num_neurons_pe);

    // Initializing NeuronLP from parameters defined by the
    initialize_NeuronLP(neuronLP);
    neuronLP->doryta_id = settings.gid_to_doryta_id(lp->gid);
    neuronLP->neuron_struct = settings.neurons[local_id];

    // Copying synapses weights from data passed in settings
    if (settings.synapses != NULL) {
        neuronLP->to_contact = settings.synapses[local_id];
    }

    // Creating spike events
    if (settings.spikes != NULL && settings.spikes[local_id] != NULL) {
        struct StorableSpike * spikes_for_neuron = settings.spikes[local_id];
        // spikes is a pointer to an array of NULL/zero terminated spikes
        while(spikes_for_neuron->intensity != 0) {
            assert_valid_StorableSpike(spikes_for_neuron);
            send_spike_from_StorableSpike(neuronLP, lp, spikes_for_neuron);
            spikes_for_neuron++;
        }
    }

    assert_valid_NeuronLP(neuronLP);

    if (settings.probe_events != NULL) {
        for (size_t i = 0; settings.probe_events[i] != NULL; i++) {
            settings.probe_events[i](neuronLP, NULL, lp);
        }
    }
}


// Why isn't this executed by _init_ itself? An initial heartbeat is only
// necessary on needy mode. On spike-driven mode, there's no initial spike
void driver_neuron_pre_run_needy(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    // send initial heartbeat
    send_heartbeat(neuronLP, lp);
}


// Forward event handler
void driver_neuron_event_needy(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    (void) bit_field;
    assert_valid_Message(msg);

    msg->time_processed = tw_now(lp);
    settings.store_neuron(neuronLP->neuron_struct, msg->reserved_for_reverse);

    switch (msg->type) {
        case MESSAGE_TYPE_heartbeat: {
            // The order of leak and fire matters. In LIF, the leak operation
            // (the second degree equation) updates the potential in the
            // neuron. Leak cannot be performed after fire because the state of
            // the neuron is unchanged until leak is executed.
            // A different ordering (fire before leak) might be necessary for
            // some neuron type but that hasn't happened yet.
            settings.neuron_leak(neuronLP->neuron_struct, settings.beat);
            bool const fired = settings.neuron_fire(neuronLP->neuron_struct);
            if (fired) {
                send_spike(neuronLP, lp, firing_delay_double);
                msg->fired = true;
            }
            send_heartbeat(neuronLP, lp);
            break;
        }

        case MESSAGE_TYPE_spike:
            settings.neuron_integrate(neuronLP->neuron_struct, msg->spike_current);
            break;
    }
}


// Reverse Event Handler
// Notice that all operations are reversed using the data stored in either the reverse
// message or the bit field
void driver_neuron_event_reverse_needy(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    (void) bit_field;
    (void) lp;
    settings.reverse_store_neuron(neuronLP->neuron_struct, msg->reserved_for_reverse);
    if (msg->type == MESSAGE_TYPE_heartbeat) {
        msg->fired = false;
    }
    msg->time_processed = -1;
}


static inline double find_prev_heartbeat_time(double now) {
    double intpart;
    modf(now / settings.beat, &intpart);
    return intpart * settings.beat;
}


// Forward event handler
void driver_neuron_event_spike_driven(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    assert(settings.neuron_leak_bigdt != NULL);
    assert_valid_Message(msg);

    bit_field->c0 = neuronLP->next_heartbeat_sent;

    msg->prev_heartbeat = neuronLP->last_heartbeat;
    msg->time_processed = tw_now(lp);
    settings.store_neuron(neuronLP->neuron_struct, msg->reserved_for_reverse);

    switch (msg->type) {
        case MESSAGE_TYPE_heartbeat: {
            // Same as needy mode, except for `last_heartbeat`
            settings.neuron_leak(neuronLP->neuron_struct, settings.beat);
            bool const fired = settings.neuron_fire(neuronLP->neuron_struct);
            if (fired) {
                send_spike(neuronLP, lp, firing_delay_double);
                msg->fired = true;
            }
            neuronLP->last_heartbeat = tw_now(lp);
            neuronLP->next_heartbeat_sent = false;
            break;
        }

        case MESSAGE_TYPE_spike: {
            // previous heartbeat is not last heartbeat. It is the timestamp
            // for when the previous heartbeat to this spike message should
            // have been
            double const prev_heartbeat_time = find_prev_heartbeat_time(tw_now(lp));
            double const beat = settings.beat;
            assert(neuronLP->last_heartbeat <= prev_heartbeat_time);
            assert(msg->neuron_to == neuronLP->doryta_id);
            assert((uint64_t) msg->neuron_to_gid == lp->gid);

            // Getting neuron up-to-date since last heartbeat
            if (! neuronLP->next_heartbeat_sent &&
                neuronLP->last_heartbeat < prev_heartbeat_time)
            {
                double const delta = prev_heartbeat_time - neuronLP->last_heartbeat;
                settings.neuron_leak_bigdt(neuronLP->neuron_struct, delta, beat);
                neuronLP->last_heartbeat = prev_heartbeat_time;
            }

            settings.neuron_integrate(neuronLP->neuron_struct, msg->spike_current);

            if (!neuronLP->next_heartbeat_sent) {
                double const dt_to_next_beat = prev_heartbeat_time + beat - tw_now(lp);
                send_heartbeat_at(neuronLP, lp, dt_to_next_beat);
                neuronLP->next_heartbeat_sent = true;
            }
            break;
        }
    }
}


// Reverse Event Handler
void driver_neuron_event_reverse_spike_driven(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    (void) lp;
    settings.reverse_store_neuron(neuronLP->neuron_struct, msg->reserved_for_reverse);
    neuronLP->last_heartbeat = msg->prev_heartbeat;
    neuronLP->next_heartbeat_sent = bit_field->c0;
    if (msg->type == MESSAGE_TYPE_heartbeat) {
        msg->fired = false;
    }
    msg->time_processed = -1;
}


// Commit event handler
// This function is only called when it can be make sure that the message won't be
// roll back. Either the commit or reverse handler will be called, not both
void driver_neuron_event_commit(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    (void) bit_field;
    if (settings.probe_events != NULL) {
        for (size_t i = 0; settings.probe_events[i] != NULL; i++) {
            settings.probe_events[i](neuronLP, msg, lp);
        }
    }
}


// The finalization function
// Reporting any final statistics for this LP in the file previously opened
void driver_neuron_final(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    (void) lp;
    int32_t const self = neuronLP->doryta_id;
    if (settings.save_state_handler != NULL) {
        FILE * fp = settings.save_state_handler;
        fprintf(fp, "LP (neuron): %" PRIi32 ". ", self);
        if (neuronLP->last_heartbeat > 0) {
            fprintf(fp, "Last heartbeat: %f ", neuronLP->last_heartbeat);
        }
        if (settings.print_neuron_struct != NULL) {
            settings.print_neuron_struct(fp, neuronLP->neuron_struct);
        }
        fprintf(fp, "\n");

        if (neuronLP->to_contact.num > 0) {
            fprintf(fp, "LP (neuron): %" PRIi32 ". Synapses:", self);
            for (int i = 0; i < neuronLP->to_contact.num; i++) {
                fprintf(fp, " %" PRIi32 ": %f",
                        neuronLP->to_contact.synapses[i].doryta_id_to_send,
                        neuronLP->to_contact.synapses[i].weight);
                if (i < neuronLP->to_contact.num - 1) {
                    fprintf(fp, ",");
                }
            }
            fprintf(fp, "\n");
        }
    }

    if (settings.neurons == NULL) {
        free(neuronLP->neuron_struct);
    }
}

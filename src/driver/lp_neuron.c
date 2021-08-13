#include "lp_neuron.h"
#include "../message.h"
#include "../mapping.h"
#include <stdbool.h>
#include <ross.h>

struct SettingsPE settings = {0};
bool settings_initialized = false;


void neuron_pe_config(struct SettingsPE settings_in) {
    assert(settings_in.neuron_leak != NULL);
    assert(settings_in.neuron_integrate != NULL);
    assert(settings_in.neuron_fire != NULL);
    settings = settings_in;
    settings_initialized = true;
}


// LP initialization. Called once for each LP
void neuronLP_init(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    assert(settings_initialized);
    uint64_t const neuron_id = get_neuron_id(lp);
    assert(neuron_id < settings.num_neurons);
    neuronLP->neuron_struct = settings.neurons[neuron_id];
}


static void send_heartbeat(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    (void) neuronLP;
    (void) lp;
    assert(false);
    // TODO: FILL ME. I should use the list of neurons the
    // neuronLP is connected to and add them stuff
}


static void send_spike(struct NeuronLP *neuronLP, struct tw_lp *lp) {
    (void) neuronLP;
    (void) lp;
    assert(false);
    // TODO: FILL ME. I should use the list of neurons the
    // neuronLP is connected to and add them stuff
}


// Forward event handler
void neuronLP_event(
        struct NeuronLP *neuronLP,
        struct tw_bf *bit_field,
        struct Message *msg,
        struct tw_lp *lp) {
    (void) bit_field;

    msg->time_processed = tw_now(lp);

    switch (msg->type) {
        case MESSAGE_TYPE_heartbeat:
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
            record_fun(neuronLP->neuron_struct, msg, lp);
        }
    }
}


// The finalization function
// Reporting any final statistics for this LP in the file previously opened
//void neuronLP_final(struct NeuronLP *neuronLP, struct tw_lp *lp) {
//}

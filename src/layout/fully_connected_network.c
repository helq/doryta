#include "fully_connected_network.h"
#include "master.h"
#include <ross.h>

void layout_fcn_reserve(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe) {
    layout_master_reserve_neurons(total_neurons, initial_pe, final_pe, LAYOUT_TYPE_fully_conn_net);
    // TODO: determine how many neurons have been
    // defined for the layout level just defined and ask
    // for the appropiate number of synapses
}

void layout_fcn_init(neuron_init_f neuron_init, synapse_init_f synapse_init) {
    (void) neuron_init;
    (void) synapse_init;
    tw_error(TW_LOC, "Not yet implemented :(");
}

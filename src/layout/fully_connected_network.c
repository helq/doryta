#include "fully_connected_network.h"
#include "master.h"
#include <ross.h>

static int num_fcn = 0;


void layout_fcn_reserve(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe) {
    int level = layout_master_reserve_neurons(total_neurons, initial_pe, final_pe, LAYOUT_TYPE_fully_conn_net);
    size_t neurons_in_pe = layout_master_neurons_in_pe_level(level);
    size_t num_synapses = total_neurons * neurons_in_pe;
    layout_master_reserve_synapses(level, num_synapses);
    num_fcn++;
}


void layout_fcn_init(neuron_init_f neuron_init, synapse_init_f synapse_init) {
    struct LayoutLevelParams fcn_level_params[num_fcn];
    layout_master_get_all_layouts(fcn_level_params, LAYOUT_TYPE_fully_conn_net);

    struct MemoryAllocationLayout const allocation =
        layout_master_allocation();

    // Going through each (FCN) layout level
    for (int i = 0; i < num_fcn; i++) {
        size_t const total_neurons = fcn_level_params[i].total_neurons;
        size_t const neurons_in_pe = fcn_level_params[i].neurons_in_pe;
        size_t const synapses_offset = fcn_level_params[i].synapses_offset;
        size_t const doryta_id_offset = fcn_level_params[i].doryta_id_offset;
        size_t const local_id_offset = fcn_level_params[i].local_id_offset;

        for (size_t j = 0; j < neurons_in_pe; j++) {
            // Connecting synapses correctly
            allocation.synapses[j].synapses =
                &allocation.naked_synapses[synapses_offset + j * total_neurons];
            allocation.synapses[j].num = total_neurons;

            size_t const doryta_id = doryta_id_offset + j;

            // Initializing synapses
            if (synapse_init != NULL) {
                for (size_t k = 0; k < total_neurons; k++) {
                    allocation.synapses[j].synapses[k].weight =
                        synapse_init(doryta_id, k);
                    size_t const gid = layout_master_doryta_id_to_gid(doryta_id);
                    allocation.synapses[j].synapses[k].gid_to_send = gid;
                    allocation.synapses[j].synapses[k].doryta_id_to_send = doryta_id;
                }
            }

            // Initializing neurons
            if (neuron_init != NULL) {
                neuron_init(allocation.neurons[local_id_offset + j], doryta_id);
            }
        }
    }
}

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
        size_t const global_neuron_offset = fcn_level_params[i].global_neuron_offset;

        for (size_t j = 0; j < neurons_in_pe; j++) {
            // Connecting synapses correctly
            allocation.synapses[local_id_offset + j].synapses =
                &allocation.naked_synapses[synapses_offset + j * total_neurons];
            allocation.synapses[local_id_offset + j].num = total_neurons;

            size_t const doryta_id = doryta_id_offset + j;

            // Initializing synapses
            if (synapse_init != NULL) {
                struct Synapse *synapses_layer =
                    allocation.synapses[local_id_offset + j].synapses;
                for (size_t k = 0; k < total_neurons; k++) {
                    size_t const to_doryta_id = global_neuron_offset + k;
                    synapses_layer[k].weight =
                        synapse_init(doryta_id, to_doryta_id);
                    synapses_layer[k].doryta_id_to_send = to_doryta_id;
                    synapses_layer[k].gid_to_send =
                        layout_master_doryta_id_to_gid(to_doryta_id);

                    /*
                     *float const weight = synapses_layer[k].weight;
                     *size_t const gid_to_send = synapses_layer[k].gid_to_send;
                     *printf("PE %lu : Initializing neuron %lu (gid: %lu)"
                     *       " with synapse to %lu (gid: %lu) -> weight = %f\n",
                     *        g_tw_mynode, doryta_id, layout_master_doryta_id_to_gid(doryta_id),
                     *        to_doryta_id, gid_to_send, weight);
                     */
                }
            }

            // Initializing neurons
            if (neuron_init != NULL) {
                neuron_init(allocation.neurons[local_id_offset + j], doryta_id);
            }
        }
    }
}

#include "standard_layouts.h"
#include "master.h"

void layout_std_fully_connected_network(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe) {
    size_t start_id = layout_master_neurons(total_neurons, initial_pe, final_pe);
    layout_master_synapses_fully(
            start_id, start_id + total_neurons - 1,
            start_id, start_id + total_neurons - 1);
}

void layout_std_fully_connected_layer(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe) {
    struct NeuronGroupInfo last = layout_master_info_latest_group();
    size_t start_id = layout_master_neurons(total_neurons, initial_pe, final_pe);
    layout_master_synapses_fully(
            last.id_offset, last.id_offset + last.num_neurons - 1,
            start_id, start_id + total_neurons - 1);
}

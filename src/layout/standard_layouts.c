#include "standard_layouts.h"
#include "master.h"

void layout_fcn_reserve(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe) {
    size_t start_id = layout_master_reserve_neurons(total_neurons, initial_pe, final_pe);
    layout_master_synapses_fully(
            start_id, start_id + total_neurons - 1,
            start_id, start_id + total_neurons - 1);
}

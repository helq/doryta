#include "load_spikes.h"
#include <stdlib.h>
#include <stdio.h>
#include "../../driver/neuron_lp.h"
#include "../../storable_spikes.h"
#include "../../layout/master.h"
#include "../../utils/io.h"

static struct StorableSpike **spikes = NULL;
static struct StorableSpike *naked_spikes = NULL;


void model_load_spikes_init(struct SettingsNeuronLP * settings_neuron_lp,
        char const filename[]) {
    FILE * fp = fopen(filename, "rb");
    if (!fp) {
        tw_error(TW_LOC, "File `%s` could not be read", filename);
    }
    uint32_t magic = load_uint32(fp);
    if (magic != 0x23432BC5) {
        tw_error(TW_LOC, "Input file corrupt or unknown (note: incorrect magic number)");
    }
    uint16_t format = load_uint16(fp);
    if (format != 0x1) {
        tw_error(TW_LOC, "Input file corrupt or format unknown");
    }
    int32_t spike_times = load_int32(fp);
    if (spike_times != 1) {
        tw_error(TW_LOC, "Sorry, but we don't yet handle more than one instant in time");
    }

    // TODO: this is the maximum number of spikes to load in a single PE. It's
    // exceedingly wasteful. It's the simplest and fastest procedure we have so
    // far
    int32_t total_spikes = load_int32(fp);
    float spikes_time = load_float(fp);
    int32_t neurons_in_batch = load_int32(fp);
    // TODO: This is only true when spike_times == 1
    assert(total_spikes == neurons_in_batch);

    int const num_neurons_pe = layout_master_total_neurons_pe();
    spikes = calloc(num_neurons_pe, sizeof(struct StorableSpike*));

    naked_spikes = calloc(2*total_spikes, sizeof(struct StorableSpike));
    int j = 0;
    for (int32_t i = 0; i < neurons_in_batch; i++) {
        int32_t neuron_i = load_int32(fp);
        if (layout_master_doryta_id_to_pe(neuron_i) == g_tw_mynode) {
            size_t local_id = layout_master_doryta_id_to_local_id(neuron_i);
            spikes[local_id] = naked_spikes + 2*j;
            spikes[local_id]->neuron = neuron_i;
            spikes[local_id]->time = spikes_time;
            spikes[local_id]->intensity = 1;
            j++;
        }
    }
    settings_neuron_lp->spikes = spikes;
}


void model_load_spikes_deinit(void) {
    free(naked_spikes);
    free(spikes);
}

#include "load_spikes.h"
#include <stdlib.h>
#include <stdio.h>
#include "../../driver/neuron.h"
#include "../../storable_spikes.h"
#include "../strategy/modes.h"
#include "../../utils/io.h"

static struct StorableSpike **spikes = NULL;
static struct StorableSpike *naked_spikes = NULL;


static inline void load_format_1(FILE * fp);
static inline void load_format_2(FILE * fp);

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
    if (format == 0x1) {
        load_format_1(fp);
    } else if (format == 0x2) {
        load_format_2(fp);
    } else {
        // TODO: New format that extends 0x2 with optional spike intensity and
        // explicitely stores the biggest possible id the data can have
        tw_error(TW_LOC, "Input file corrupt or format unknown");
    }
    settings_neuron_lp->spikes = spikes;
}


/* Loading format 1, which stores neurons per each timestamp (spike time).
 */
static inline void load_format_1(FILE * fp) {
    int32_t const spike_times = load_int32(fp);
    if (spike_times != 1) {
        tw_error(TW_LOC, "Sorry, but we don't yet handle more than one instant in time");
    }
    struct StrategyParams const loadingModeParams = get_model_strategy_mode();

    int32_t const total_spikes = load_int32(fp);

    float const spikes_time = load_float(fp);
    int32_t const neurons_in_batch = load_int32(fp);
    // TODO: This is only true when spike_times == 1
    assert(total_spikes == neurons_in_batch);

    int const num_neurons_pe = loadingModeParams.lps_in_pe();
    spikes = calloc(num_neurons_pe, sizeof(struct StorableSpike*));

    // TODO: this is the maximum number of spikes to load in a single PE. It's
    // exceedingly wasteful. It's the simplest and fastest procedure we have so
    // far
    naked_spikes = calloc(2*total_spikes, sizeof(struct StorableSpike));
    int j = 0;
    for (int32_t i = 0; i < neurons_in_batch; i++) {
        int32_t const neuron_i = load_int32(fp);
        // If current neuron (neuron_i) is in PE
        if (loadingModeParams.doryta_id_to_pe(neuron_i) == g_tw_mynode) {
            size_t const local_id = loadingModeParams.doryta_id_to_local_id(neuron_i);
            spikes[local_id] = naked_spikes + 2*j;
            spikes[local_id]->neuron = neuron_i;
            spikes[local_id]->time = spikes_time;
            spikes[local_id]->intensity = 1;
            j++;
        }
    }
}

/* Loading format 2, which stores spikes for each neuron.
 */
static inline void load_format_2(FILE * fp) {
    struct StrategyParams const loadingModeParams = get_model_strategy_mode();

    int32_t const total_neurons = load_int32(fp);
    int32_t const total_spikes = load_int32(fp);

    int const num_neurons_pe = loadingModeParams.lps_in_pe();
    spikes = calloc(num_neurons_pe, sizeof(struct StorableSpike*));

    // TODO: `spikes_in_pe` does not actually contain the number of spikes to store in PE.
    // This is an upper limit and it's very wasteful, but it's easy to compute.
    // In the future there could be two loops, one computing the actual amount
    // of necessary memory and another loading that into memory, or a better
    // file structure. Check `j` variable below
    int32_t const spikes_in_pe = total_spikes
        + (total_neurons < num_neurons_pe ? total_neurons : num_neurons_pe);
    naked_spikes = calloc(spikes_in_pe, sizeof(struct StorableSpike));

    int j = 0;
    for (int32_t i = 0; i < total_neurons; i++) {
        int32_t const neuron_i = load_int32(fp);
        int32_t const num_spikes = load_int32(fp);
        // If current neuron (neuron_i) is in PE
        if (loadingModeParams.doryta_id_to_pe(neuron_i) == g_tw_mynode) {
            float spikes_raw[num_spikes];
            load_floats(fp, spikes_raw, num_spikes);
            size_t const local_id = loadingModeParams.doryta_id_to_local_id(neuron_i);

            spikes[local_id] = naked_spikes + j;
            for (int k = 0; k < num_spikes; k++) {
                spikes[local_id][k].neuron = neuron_i;
                spikes[local_id][k].time = spikes_raw[k];
                spikes[local_id][k].intensity = 1;
            }
            j += num_spikes + 1; // One extra empty spike
        } else {
            fseek(fp, num_spikes * sizeof(float), SEEK_CUR);
        }
    }
    // At the end of the iteration, j contains the actual space used by the
    // current PE
    assert(j <= spikes_in_pe);
}


void model_load_spikes_deinit(void) {
    free(naked_spikes);
    free(spikes);
}

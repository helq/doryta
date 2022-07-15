#define _DEFAULT_SOURCE

#include "random_spikes.h"
#include <stdlib.h>
#include <stdio.h>
#include <pcg_basic.h>
#include "../../../storable_spikes.h"
#include "../../strategy/layouts/master.h"
#include "../../strategy/modes.h"
#include "../../../utils/io_load.h"
#include "../../../utils/pcg32_random.h"

static struct StorableSpike **spikes = NULL;
static struct StorableSpike *naked_spikes = NULL;


void model_random_spikes_init(struct SettingsNeuronLP * settings_neuron_lp,
        double prob, double spikes_time, unsigned int until) {
    struct StrategyParams const loadingModeParams = get_model_strategy_mode();

    int const num_neurons_pe = loadingModeParams.lps_in_pe();
    spikes = calloc(num_neurons_pe, sizeof(struct StorableSpike*));
    naked_spikes = calloc(2*num_neurons_pe, sizeof(struct StorableSpike));
    for (int32_t i = 0; i < num_neurons_pe; i++) {
        size_t const doryta_id = loadingModeParams.local_id_to_doryta_id(i);

        if (doryta_id >= until) {
            continue;
       }

        pcg32_random_t rng;
        uint32_t const initstate = doryta_id + 42u;
        uint32_t const initseq = doryta_id + 54u;
        pcg32_srandom_r(&rng, initstate, initseq);

        // Yes, we are casting `float`s as `double`s, but it is not a big deal.
        // The probability won't be affected
        double const random_double = pcg32_float_r(&rng);
        // If current neuron (neuron_i) is in PE
        if (random_double <= prob) {
            spikes[i] = naked_spikes + 2*i;
            spikes[i]->neuron = doryta_id;
            spikes[i]->time = spikes_time;
            spikes[i]->intensity = 1;
        }
    }
    settings_neuron_lp->spikes = spikes;
}


void model_random_spikes_deinit(void) {
    free(naked_spikes);
    free(spikes);
}

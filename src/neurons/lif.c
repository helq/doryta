#include "lif.h"
#include <stdio.h>

void leak_lif_neuron(struct LifNeuron * lf, float dt) {
    lf->potential = lf->potential
        + dt * (- lf->potential + lf->resting_potential
                + lf->current * lf->resistance) / lf->tau_m;
}


void leak_lif_big_neuron(struct LifNeuron * lf, float delta, float dt) {
    size_t iterations = delta / dt;
    if (lf->current == 0 && lf->potential == lf->resting_potential) {
        // Neuron already arrived at resting potential, no need to compute
        // anything
        return;
    }
    for (size_t i = 0; i < iterations; i++) {
        lf->potential = lf->potential
            + dt * (- lf->potential + lf->resting_potential
                    + lf->current * lf->resistance) / lf->tau_m;
    }
}


void integrate_lif_neuron(struct LifNeuron * lf, float spike_current) {
    /*if (spike_current > 0) */
    lf->current += spike_current;
}


bool fire_lif_neuron(struct LifNeuron * lf) {
    bool const to_fire = lf->potential > lf->threshold;
    if (to_fire) {
        lf->potential = lf->reset_potential;
    }
    // The current resets on every instant in time
    // It is equal to whatever it gets from spikes
    lf->current = 0;
    return to_fire;
}


void store_lif_neuron_state(
        struct LifNeuron * lf,
        struct StorageInMessageLif * storage) {
    storage->potential = lf->potential;
    storage->current = lf->current;
}


void reverse_store_lif_neuron_state(
        struct LifNeuron * lf,
        struct StorageInMessageLif * storage) {
    lf->potential = storage->potential;
    lf->current = storage->current;
}


void print_lif_neuron(struct LifNeuron * lif) {
    printf("potential = %f "
           "current = %f "
           "resting_potential = %f "
           "threshold = %f "
           "tau_m = %f "
           "resistance = %f\n",
           lif->potential,
           lif->current,
           lif->resting_potential,
           lif->threshold,
           lif->tau_m,
           lif->resistance);
}

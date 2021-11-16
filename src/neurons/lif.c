#include "lif.h"
#include <stdio.h>
#include <tgmath.h>

void leak_lif_neuron(struct LifNeuron * lf, double dt) {
    // V(t + dt) = V(t) + dt * (-(V(t) - Ve) + I(t) R / (R * C))
    lf->potential = lf->potential
        + dt * (- lf->potential + lf->resting_potential
                + lf->current * lf->resistance) / lf->tau_m;
}


void leak_lif_big_neuron(struct LifNeuron * lf, double delta, double dt) {
    (void) dt;
    assert(lf->current == 0);
    assert(lf->threshold > lf->resting_potential);
    // V(t) = Ve + exp(-t / (R * C)) * (Vi - Ve)
    lf->potential = lf->resting_potential
        + exp(- delta / lf->tau_m) * (lf->potential - lf->resting_potential);
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

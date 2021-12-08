#include "lif.h"
#include <stdio.h>
#include <tgmath.h>

void neurons_lif_leak(struct LifNeuron * lf, double dt) {
    // V(t + dt) = V(t) + dt * (-(V(t) - Ve) + I(t) R / (R * C))
    lf->potential = lf->potential
        + dt * (- lf->potential + lf->resting_potential
                + lf->current * lf->resistance) / lf->tau_m;
}


void neurons_lif_big_leak(struct LifNeuron * lf, double delta, double dt) {
    (void) dt;
    assert(lf->current == 0);
    assert(lf->threshold > lf->resting_potential);
    // V(t) = Ve + exp(-t / (R * C)) * (Vi - Ve)
    lf->potential = lf->resting_potential
        + exp(- delta / lf->tau_m) * (lf->potential - lf->resting_potential);
}


void neurons_lif_integrate(struct LifNeuron * lf, float spike_current) {
    /*if (spike_current > 0) */
    lf->current += spike_current;
}


bool neurons_lif_fire(struct LifNeuron * lf) {
    bool const to_fire = lf->potential > lf->threshold;
    if (to_fire) {
        lf->potential = lf->reset_potential;
    }
    // The current resets on every instant in time
    // It is equal to whatever it gets from spikes
    lf->current = 0;
    return to_fire;
}


void neurons_lif_store_state(
        struct LifNeuron * lf,
        struct StorageInMessageLif * storage) {
    storage->potential = lf->potential;
    storage->current = lf->current;
}


void neurons_lif_reverse_store_state(
        struct LifNeuron * lf,
        struct StorageInMessageLif * storage) {
    lf->potential = storage->potential;
    lf->current = storage->current;
}


void neurons_lif_print(FILE * fp, struct LifNeuron * lif) {
    fprintf(fp,
           "potential = %f "
           "current = %f "
           "resting_potential = %f "
           "threshold = %f "
           "tau_m = %f "
           "resistance = %f",
           lif->potential,
           lif->current,
           lif->resting_potential,
           lif->threshold,
           lif->tau_m,
           lif->resistance);
}

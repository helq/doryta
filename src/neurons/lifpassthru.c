#include "lifpassthru.h"
#include <stdio.h>
#include <tgmath.h>

void neurons_lifpassthru_leak(struct LifPassthruNeuron * lf, double dt) {
    // V(t + dt) = V(t) + dt * (-(V(t) - Ve) + I(t) R / (R * C))
    lf->potential = lf->potential
        + dt * (- lf->potential + lf->resting_potential
                + lf->current * lf->resistance) / lf->tau_m;
}


void neurons_lifpassthru_big_leak(struct LifPassthruNeuron * lf, double delta, double dt) {
    (void) dt;
    assert(lf->current == 0);
    assert(lf->threshold > lf->resting_potential);
    // V(t) = Ve + exp(-t / (R * C)) * (Vi - Ve)
    lf->potential = lf->resting_potential
        + exp(- delta / lf->tau_m) * (lf->potential - lf->resting_potential);
}


void neurons_lifpassthru_integrate(struct LifPassthruNeuron * lf, float spike_current) {
    /*if (spike_current > 0) */
    lf->current += spike_current;
    lf->activated = true;
}


struct NeuronFiring neurons_lifpassthru_fire(struct LifPassthruNeuron * lf) {
    if (lf->passthru) {
        bool const to_fire = lf->activated;
        double const intensity = lf->potential;
        lf->activated = false;
        lf->potential = lf->reset_potential;
        lf->current = 0;
        return (struct NeuronFiring) {to_fire, intensity};

    } else {
        bool const to_fire = lf->potential > lf->threshold;
        if (to_fire) {
            lf->potential = lf->reset_potential;
        }
        // The current resets on every instant in time
        // It is equal to whatever it gets from spikes
        lf->current = 0;
        return (struct NeuronFiring) {to_fire, 1.0};
    }
}


void neurons_lifpassthru_store_state(
        struct LifPassthruNeuron * lf,
        struct StorageInMessageLifPassthru * storage) {
    storage->potential = lf->potential;
    storage->current = lf->current;
}


void neurons_lifpassthru_reverse_store_state(
        struct LifPassthruNeuron * lf,
        struct StorageInMessageLifPassthru * storage) {
    lf->potential = storage->potential;
    lf->current = storage->current;
}


void neurons_lifpassthru_print(FILE * fp, struct LifPassthruNeuron * lif) {
    fprintf(fp,
           "potential = %f "
           "current = %f "
           "resting_potential = %f "
           "threshold = %f "
           "tau_m = %f "
           "resistance = %f "
           "passthru = %d",
           lif->potential,
           lif->current,
           lif->resting_potential,
           lif->threshold,
           lif->tau_m,
           lif->resistance,
           lif->passthru);
}

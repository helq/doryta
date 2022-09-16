#include "lif_beta.h"
#include <stdio.h>

void neurons_lif_beta_leak(struct LifBetaNeuron * lf, double dt) {
    (void) dt;
    lf->potential = lf->beta * lf->potential;
}


void neurons_lif_beta_integrate(struct LifBetaNeuron * lf, float current) {
    lf->potential += current;
}


struct NeuronFiring neurons_lif_beta_fire(struct LifBetaNeuron * lf) {
    bool const to_fire = lf->potential > lf->threshold;
    if (to_fire) {
        lf->potential = lf->baseline;
    }
    return (struct NeuronFiring) {to_fire, 1.0};
}


void neurons_lif_beta_store_state(
        struct LifBetaNeuron * lf,
        struct StorageInMessageLifBeta * storage) {
    storage->potential = lf->potential;
}


void neurons_lif_beta_reverse_store_state(
        struct LifBetaNeuron * lf,
        struct StorageInMessageLifBeta * storage) {
    lf->potential = storage->potential;
}


void neurons_lif_beta_print(FILE * fp, struct LifBetaNeuron * lif) {
    fprintf(fp, "potential = %f  threshold = %f  beta = %f  baseline = %f\n",
            lif->potential, lif->threshold, lif->beta, lif->baseline);
}

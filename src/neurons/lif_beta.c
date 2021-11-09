#include "lif_beta.h"
#include <stdio.h>

void leak_lif_beta_neuron(struct LifBetaNeuron * lf, double dt) {
    (void) dt;
    lf->potential = lf->beta * lf->potential;
}


void integrate_lif_beta_neuron(struct LifBetaNeuron * lf, float current) {
    lf->potential += current;
}


bool fire_lif_beta_neuron(struct LifBetaNeuron * lf) {
    bool const to_fire = lf->potential > lf->threshold;
    if (to_fire) {
        lf->potential = lf->baseline;
    }
    return to_fire;
}


void store_lif_beta_neuron_state(
        struct LifBetaNeuron * lf,
        struct StorageInMessageLifBeta * storage) {
    storage->potential = lf->potential;
}


void reverse_store_lif_beta_neuron_state(
        struct LifBetaNeuron * lf,
        struct StorageInMessageLifBeta * storage) {
    lf->potential = storage->potential;
}


void print_lif_beta_neuron(struct LifBetaNeuron * lif) {
    printf("potential = %f  threshold = %f  beta = %f  baseline = %f\n",
            lif->potential, lif->threshold, lif->beta, lif->baseline);
}

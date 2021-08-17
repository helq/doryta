#include "lif_beta.h"

void leak_lif_neuron(struct LifNeuron * lf) {
    lf->potential = lf->beta * lf->potential;
}

void integrate_lif_neuron(struct LifNeuron * lf, float current) {
    lf->potential += current;
}

bool fire_lif_neuron(struct LifNeuron * lf) {
    bool const to_fire = lf->potential > lf->threshhold;
    if (to_fire) {
        lf->potential = lf->baseline;
    }
    return to_fire;
}

#include "lif_beta.h"

void leak_lif_neuron(struct LifNeuron * lf, float dt) {
    (void) dt;
    lf->potential = lf->beta * lf->potential;
}


void integrate_lif_neuron(struct LifNeuron * lf, float current) {
    lf->potential += current;
}


bool fire_lif_neuron(struct LifNeuron * lf) {
    bool const to_fire = lf->potential > lf->threshold;
    if (to_fire) {
        lf->potential = lf->baseline;
    }
    return to_fire;
}


void store_lif_neuron_state(struct LifNeuron * lf, void * storage_space) {
    struct StorageInMessage * storage =
        (struct StorageInMessage *) storage_space;
    storage->potential = lf->potential;
}


void reverse_store_lif_neuron_state(
        struct LifNeuron * lf, void * storage_space) {
    struct StorageInMessage * storage =
        (struct StorageInMessage *) storage_space;
    lf->potential = storage->potential;
}

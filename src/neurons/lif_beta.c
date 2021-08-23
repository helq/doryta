#include "lif_beta.h"
#include <string.h>
#include <assert.h>

void leak_lif_neuron(struct LifNeuron * lf) {
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


union StorageAndLif {
    char binary[32];
    struct { // data to store
        float potential;
    };
};
static_assert(sizeof(union StorageAndLif) == 32,
        "The data to store in the `char[32]` cannot exceed 32 bytes");


void store_lif_neuron_state(struct LifNeuron * lf, char storage_space[32]) {
    union StorageAndLif * storage = (union StorageAndLif *) storage_space;
    storage->potential = lf->potential;
}


void reverse_store_lif_neuron_state(
        struct LifNeuron * lf, char storage_space[32]) {
    union StorageAndLif * storage = (union StorageAndLif *) storage_space;
    lf->potential = storage->potential;
}

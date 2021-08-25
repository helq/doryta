#ifndef DORYTA_NEOURNS_LIF_BETA_H
#define DORYTA_NEOURNS_LIF_BETA_H

#include "../message.h"
#include <stdbool.h>
#include <assert.h>

// This header file must define at least three functions to be
// called later by the neuron handler:
// - Leak
// - Fire
// - Integrate
// Additionally it should define a struct that contains the values
// that those functions apply
// This file is, for now, included directly by lp_neuron.c, but later
// it should be defined by an interface defined by lp_neuron
//

/** This very simple LIF implementation requires all values to be positive.
 * Invariants:
 * - params are positive
 */
struct LifNeuron {
    float potential;
    float threshold;
    float beta;
    float baseline;
};


/** This struct determines how to store data inside the `reserved_for_reverse`
 * variable in `Message`. To be used by `store_lif_neuron_state`,
 * `reverse_store_lif_neuron_state` and any function which wishes to access to
 * the state of the message BEFORE it was processed!
 *
 * Remember that the data used to reverse the state of the neuron cannot be
 * bigger than MESSAGE_SIZE_REVERSE
 */
struct StorageInMessage {
    float potential;
};
#define STRING_HELPER(x) #x
#define WARNING_MESSAGE(x) \
    "The data to store in the `char[" STRING_HELPER(x) \
    "]` cannot exceed " STRING_HELPER(x) " bytes"
static_assert(sizeof(struct StorageInMessage) <= MESSAGE_SIZE_REVERSE,
        WARNING_MESSAGE(MESSAGE_SIZE_REVERSE));


void leak_lif_neuron(struct LifNeuron *, float);

void integrate_lif_neuron(struct LifNeuron *, float current);

bool fire_lif_neuron(struct LifNeuron *);

void store_lif_neuron_state(struct LifNeuron *, void *);

void reverse_store_lif_neuron_state(struct LifNeuron *, void *);

#endif /* end of include guard */

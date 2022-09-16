#ifndef DORYTA_NEOURNS_LIF_BETA_H
#define DORYTA_NEOURNS_LIF_BETA_H

#include "../message.h"
#include <stdio.h>

// This header file must define at least three functions to be
// called later by the neuron handler:
// - Leak
// - Fire
// - Integrate
// Additionally it should define a struct that contains the values
// that those functions apply
// This file is, for now, included directly by neuron_lp.c, but later
// it should be defined by an interface defined by neuron_lp
//

/** This very simple LIF implementation requires all values to be positive.
 * Invariants:
 * - params are positive
 */
struct LifBetaNeuron {
    float potential;
    float threshold;
    float beta;
    float baseline;
};


/** This struct determines how to store data inside the `reserved_for_reverse`
 * variable in `Message`. To be used by `neurons_lif_beta_store_state`,
 * `neurons_lif_beta_reverse_store_state` and any function which wishes to
 * access to the state of the message BEFORE it was processed!
 *
 * Remember that the data used to reverse the state of the neuron cannot be
 * bigger than MESSAGE_SIZE_REVERSE
 */
struct StorageInMessageLifBeta {
    float potential;
};
#define STRING_HELPER(x) #x
#define WARNING_MESSAGE(x) \
    "The data to store in the `char[" STRING_HELPER(x) \
    "]` cannot exceed " STRING_HELPER(x) " bytes"
static_assert(sizeof(struct StorageInMessageLifBeta) <= MESSAGE_SIZE_REVERSE,
        WARNING_MESSAGE(MESSAGE_SIZE_REVERSE));


void neurons_lif_beta_leak(struct LifBetaNeuron *, double);

void neurons_lif_beta_integrate(struct LifBetaNeuron *, float current);

struct NeuronFiring neurons_lif_beta_fire(struct LifBetaNeuron *);

void neurons_lif_beta_store_state(
        struct LifBetaNeuron *,
        struct StorageInMessageLifBeta *);

void neurons_lif_beta_reverse_store_state(
        struct LifBetaNeuron *,
        struct StorageInMessageLifBeta *);

void neurons_lif_beta_print(FILE * fp, struct LifBetaNeuron *);

#endif /* end of include guard */

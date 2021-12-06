#ifndef DORYTA_NEOURNS_LIF_H
#define DORYTA_NEOURNS_LIF_H

#include "../message.h"

// This header file must define at least three functions to be
// called later by the neuron handler:
// - Leak
// - Fire
// - Integrate
// Additionally it should define a struct that contains the values
// that those functions apply
// This file is, for now, included directly by neuron_lp.c, but later
// it should be defined by an interface defined by neuron_lp

/** This very simple LIF implementation requires all values to be positive.
 * Invariants:
 * - params are positive
 */
struct LifNeuron {
    float potential;          // V
    float current;            // I(t)
    float resting_potential;  // V_e
    float reset_potential;    // V_r
    float threshold;          // V_th
    float tau_m;              // R * C
    float resistance;         // R
};


/** This struct determines how to store data inside the `reserved_for_reverse`
 * variable in `Message`. To be used by neurons_lif_`store_state`,
 * neurons_lif_`reverse_store_state` and any function which wishes to access to
 * the state of the message BEFORE it was processed!
 *
 * Remember that the data used to reverse the state of the neuron cannot be
 * bigger than MESSAGE_SIZE_REVERSE
 */
struct StorageInMessageLif {
    float potential;
    float current;
};
#define STRING_HELPER(x) #x
#define WARNING_MESSAGE(x) \
    "The data to store in the `char[" STRING_HELPER(x) \
    "]` cannot exceed " STRING_HELPER(x) " bytes"
static_assert(sizeof(struct StorageInMessageLif) <= MESSAGE_SIZE_REVERSE,
        WARNING_MESSAGE(MESSAGE_SIZE_REVERSE));


void neurons_lif_leak(struct LifNeuron *, double);

void neurons_lif_big_leak(struct LifNeuron *, double, double);

void neurons_lif_integrate(struct LifNeuron *, float current);

bool neurons_lif_fire(struct LifNeuron *);

void neurons_lif_store_state(
        struct LifNeuron *,
        struct StorageInMessageLif * storage);

void neurons_lif_reverse_store_state(
        struct LifNeuron *,
        struct StorageInMessageLif * storage);

void neurons_lif_print(struct LifNeuron * lif);

#endif /* end of include guard */

#ifndef DORYTA_NEOURNS_LIF_BETA_H
#define DORYTA_NEOURNS_LIF_BETA_H

#include <stdbool.h>

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
    float threshhold;
    float beta;
    float baseline;
};

void leak_lif_neuron(struct LifNeuron *);

void integrate_lif_neuron(struct LifNeuron *, float current);

bool fire_lif_neuron(struct LifNeuron *);

#endif /* end of include guard */

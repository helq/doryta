#ifndef DORYTA_PROBES_FIRING_H
#define DORYTA_PROBES_FIRING_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct NeuronLP;
struct Message;
struct tw_lp;

/** This probe records when a neuron has fired. If `only_output_neurons` is true,
 * it will only record neurons that have no output synapses to others (often, the
 * last layer of a NN).
 */
void probes_firing_init(
        size_t buffer_size, char const output_path[],
        char const filename[], bool only_output_neurons);

void probes_firing_record(struct NeuronLP *, struct Message *, struct tw_lp *);

void probes_firing_deinit(void);

#endif /* end of include guard */

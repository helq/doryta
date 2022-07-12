#ifndef DORYTA_SRC_MODEL_LOADERS_STRATEGY_LAYOUTS_STANDARD_LAYOUTS_H
#define DORYTA_SRC_MODEL_LOADERS_STRATEGY_LAYOUTS_STANDARD_LAYOUTS_H

#include <stddef.h>

/**
 * Fully Connected Layout. The number of synapses (connections) between neurons
 * is equal to `total_neurons ^ 2`.
 */
void layout_std_fully_connected_network(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe);

/**
 * Fully Connected Layer. It puts a fully connected layer on top of the latest
 * neuron group.
 */
void layout_std_fully_connected_layer(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe);

#endif /* end of include guard */

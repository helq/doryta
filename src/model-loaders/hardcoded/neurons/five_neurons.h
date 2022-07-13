#ifndef DORYTA_SRC_MODEL_LOADERS_HARDCODED_NEURONS_FIVE_NEURONS_H
#define DORYTA_SRC_MODEL_LOADERS_HARDCODED_NEURONS_FIVE_NEURONS_H

#include "../../params.h"

struct SettingsNeuronLP;

struct ModelParams model_five_neurons_init(struct SettingsNeuronLP *);

void model_five_neurons_deinit(void);

#endif /* end of include guard */

#ifndef DORYTA_SRC_MODEL_LOADERS_HARDCODED_NEURONS_GOL_NEURONS_H
#define DORYTA_SRC_MODEL_LOADERS_HARDCODED_NEURONS_GOL_NEURONS_H

#include "../../params.h"

struct SettingsNeuronLP;

struct ModelParams model_GoL_neurons_init(
        struct SettingsNeuronLP *, unsigned int width_world, double heartbeat);

void model_GoL_neurons_deinit(void);

#endif /* end of include guard */

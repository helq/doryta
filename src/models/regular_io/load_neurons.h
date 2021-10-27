#ifndef DORYTA_SRC_MODELS_REGULAR_IO_LOAD_NEURONS_H
#define DORYTA_SRC_MODELS_REGULAR_IO_LOAD_NEURONS_H

#include "../params.h"

struct SettingsNeuronLP;

struct ModelParams model_load_neurons_init(struct SettingsNeuronLP *, char const []);

void model_load_neurons_deinit(void);

#endif /* end of include guard */

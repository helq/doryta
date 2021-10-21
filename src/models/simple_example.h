#ifndef DORYTA_SRC_MODELS_SIMPLE_EXAMPLE_H
#define DORYTA_SRC_MODELS_SIMPLE_EXAMPLE_H

#include "params.h"

struct SettingsNeuronLP;

struct ModelParams model_simple_example_init(struct SettingsNeuronLP *);

void model_simple_example_deinit(void);

#endif /* end of include guard */

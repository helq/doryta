#ifndef DORYTA_SRC_MODELS_IO_LOAD_SPIKES_H
#define DORYTA_SRC_MODELS_IO_LOAD_SPIKES_H

struct SettingsNeuronLP;

void model_load_spikes_init(struct SettingsNeuronLP *, char const []);

void model_load_spikes_deinit(void);

#endif /* end of include guard */

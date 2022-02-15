#ifndef DORYTA_SRC_MODELS_HARDCODED_RANDOM_SPIKES_H
#define DORYTA_SRC_MODELS_HARDCODED_RANDOM_SPIKES_H

struct SettingsNeuronLP;

void model_random_spikes_init(struct SettingsNeuronLP *, double prob, double spikes_time, unsigned int until);

void model_random_spikes_deinit(void);

#endif /* end of include guard */

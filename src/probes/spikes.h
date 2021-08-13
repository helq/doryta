#ifndef DORYTA_PROBES_SPIKES_H
#define DORYTA_PROBES_SPIKES_H

#include <stdint.h>
#include <stddef.h>

struct NeuronLP;
struct Message;
struct tw_lp;

void initialize_record_spikes(size_t buffer_size);

void record_spikes(struct NeuronLP *, struct Message *, struct tw_lp *);

void save_record_spikes(char const *);

void deinitialize_record_spikes(void);

#endif /* end of include guard */

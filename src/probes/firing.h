#ifndef DORYTA_PROBES_SPIKES_H
#define DORYTA_PROBES_SPIKES_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct NeuronLP;
struct Message;
struct tw_lp;

void initialize_record_firing(size_t buffer_size);

void record_firing(struct NeuronLP *, struct Message *, struct tw_lp *);

void save_record_firing(char const *);

void deinitialize_record_firing(void);

#endif /* end of include guard */

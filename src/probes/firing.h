#ifndef DORYTA_PROBES_SPIKES_H
#define DORYTA_PROBES_SPIKES_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct NeuronLP;
struct Message;
struct tw_lp;

void probes_firing_init(size_t buffer_size);

void probes_firing_record(struct NeuronLP *, struct Message *, struct tw_lp *);

void probes_firing_save(char const *);

void probes_firing_deinit(void);

#endif /* end of include guard */

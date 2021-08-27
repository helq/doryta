#ifndef DORYTA_PROBES_LIF_VOLTAGE_H
#define DORYTA_PROBES_LIF_VOLTAGE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct NeuronLP;
struct Message;
struct tw_lp;

void initialize_record_lif_voltages(size_t buffer_size);

void record_lif_voltages(struct NeuronLP *, struct Message *, struct tw_lp *);

void save_record_lif_voltages(char const *);

void deinitialize_record_lif_voltages(void);

#endif /* end of include guard */

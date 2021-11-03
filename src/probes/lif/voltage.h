#ifndef DORYTA_PROBES_LIF_VOLTAGE_H
#define DORYTA_PROBES_LIF_VOLTAGE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct NeuronLP;
struct Message;
struct tw_lp;

void probes_lif_voltages_init(size_t buffer_size, char const [], char const []);

void probes_lif_voltages_record(struct NeuronLP *, struct Message *, struct tw_lp *);

void probes_lif_voltages_deinit(void);

#endif /* end of include guard */

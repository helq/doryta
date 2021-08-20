#ifndef DORYTA_PROBES_LIF_BETA_VOLTAGE_H
#define DORYTA_PROBES_LIF_BETA_VOLTAGE_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

struct NeuronLP;
struct Message;
struct tw_lp;

typedef unsigned long (*get_neuron_id_f) (struct tw_lp *);

struct VoltagesLIFbetaStngs {
    get_neuron_id_f get_neuron_id;
};

static inline bool is_valid_VoltagesLIFbetaStngs(
        struct VoltagesLIFbetaStngs * settings) {
    return settings->get_neuron_id != NULL;
}

static inline void assert_valid_VoltagesLIFbetaStngs(
        struct VoltagesLIFbetaStngs * settings) {
#ifndef NDEBUG
    assert(settings->get_neuron_id != NULL);
#endif // NDEBUG
}

void initialize_record_lif_beta_voltages(
        size_t buffer_size,
        struct VoltagesLIFbetaStngs * settings_in);

void record_lif_beta_voltages(struct NeuronLP *, struct Message *, struct tw_lp *);

void save_record_lif_beta_voltages(char const *);

void deinitialize_record_lif_beta_voltages(void);

#endif /* end of include guard */

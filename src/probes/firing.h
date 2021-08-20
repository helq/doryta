#ifndef DORYTA_PROBES_SPIKES_H
#define DORYTA_PROBES_SPIKES_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct NeuronLP;
struct Message;
struct tw_lp;

typedef unsigned long (*get_neuron_id_f) (struct tw_lp *);

struct FiringProbeSettings {
    get_neuron_id_f get_neuron_id;
};

static inline bool is_valid_FiringProbeSettings(
        struct FiringProbeSettings * settings) {
    return settings->get_neuron_id != NULL;
}

static inline void assert_valid_FiringProbeSettings(
        struct FiringProbeSettings * settings) {
#ifndef NDEBUG
    assert(settings->get_neuron_id != NULL);
#endif // NDEBUG
}

void initialize_record_firing(
        size_t buffer_size, struct FiringProbeSettings *);

void record_firing(struct NeuronLP *, struct Message *, struct tw_lp *);

void save_record_firing(char const *);

void deinitialize_record_firing(void);

#endif /* end of include guard */

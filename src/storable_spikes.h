#ifndef DORYTA_STORABLE_SPIKES_H
#define DORYTA_STORABLE_SPIKES_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

/**
 * `intensity` usually is `1`
 *
 * Invariants:
 * - `neuron` can only be positive
 * - `time` cannot be negative
 * - `intensity` cannot be zero (removing this invariant will
 *   break code, invalid, zero-ed `StorableSpike`s are used as a
 *   signal that we arrived at the last element of an array of
 *   spikes)
 */
struct StorableSpike {
    int32_t neuron;
    double time;
    double intensity;
};

static inline bool is_valid_StorableSpike(struct StorableSpike * spike) {
    return spike->neuron >= 0
        && spike->time >= 0
        && spike->intensity != 0;
}

static inline void assert_valid_StorableSpike(struct StorableSpike * spike) {
#ifndef NDEBUG
    assert(spike->neuron >= 0);
    assert(spike->time >= 0);
    assert(spike->intensity != 0);
#endif // NDEBUG
}

#endif /* end of include guard */

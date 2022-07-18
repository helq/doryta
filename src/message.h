#ifndef DORYTA_EVENT_H
#define DORYTA_EVENT_H

#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

#define MESSAGE_SIZE_REVERSE 32

// Order of types is important. An array of spikes is encoded as
// zeroed-terminated array which is only possible if the `spike`
// type is not zero
enum MESSAGE_TYPE {
    MESSAGE_TYPE_heartbeat,
    MESSAGE_TYPE_spike,
};

/**
 * Invariants:
 * - `spike_current` must be a number (not NaN)
 * - `neuron_from` must be less than the number of LPs in the system
 * - `neuron_to` must be less than the number of LPs in the system
 * - `rng_count` can be larger than zero if `fired` is true, it is zero otherwise
 */
struct Message {
    enum MESSAGE_TYPE type;
    double time_processed;
    union {
        struct { // message type = heartbeat
            bool fired;
            long rng_count;
        };
        struct { // message type = spike
#ifndef NDEBUG
            int32_t neuron_from; // DorytaID
            int32_t neuron_to;   // DorytaID
#endif
            int64_t neuron_from_gid;
            int64_t neuron_to_gid;
            float spike_current;
        };
    };
    // Reverse only fields
    double prev_heartbeat;
    // This reserves MESSAGE_SIZE_REVERSE bytes (enough for
    // MESSAGE_SIZE_REVERSE/4 double-precision floating point numbers,
    // MESSAGE_SIZE_REVERSE/8 64-bit integers, or really anything that can occupy
    // that same space. This is meant to be used by the neuron mechanism to store
    // and restore the state of the neuron.)
    char reserved_for_reverse[MESSAGE_SIZE_REVERSE];
};

// Creates a message with the appropiate fields loaded. Default values are
// given to all fields. Some default values are incorrect and must be
// overwritten by the user (eg, `neuron_from`). This function just ensures
// that all non-esential parameters are properly initialized (eg,
// `time_processed` and `fired`)
static inline void initialize_Message(struct Message * msg, enum MESSAGE_TYPE type) {
    msg->time_processed = -1;
    switch (type) {
        case MESSAGE_TYPE_heartbeat:
            msg->type = MESSAGE_TYPE_heartbeat;
            msg->fired = false;
            msg->rng_count = 0;
            break;
        case MESSAGE_TYPE_spike:
            msg->type = MESSAGE_TYPE_spike;
            msg->spike_current = 1;
#ifndef NDEBUG
            msg->neuron_from = -1;
            msg->neuron_to = -1;
#endif
            msg->neuron_from_gid = -1;
            msg->neuron_to_gid = -1;
            break;
    }
}

extern uint64_t g_tw_total_lps; //< Total number of LPs (defined by ROSS)

static inline bool is_valid_Message(struct Message * msg) {
    bool correct_spike = true;
    bool correct_heartbeat = true;
    switch (msg->type) {
        case MESSAGE_TYPE_heartbeat:
            correct_heartbeat = msg->fired ? msg->rng_count >= 0 : msg->rng_count == 0;
            break;
        case MESSAGE_TYPE_spike:
            correct_spike = correct_spike
                && !isnan(msg->spike_current)
#ifndef NDEBUG
                && 0 <= msg->neuron_from
                && 0 <= msg->neuron_to
                && (uint64_t) msg->neuron_from < g_tw_total_lps
                && (uint64_t) msg->neuron_to < g_tw_total_lps
#endif
                && 0 <= msg->neuron_from_gid
                && 0 <= msg->neuron_to_gid;
            break;
    }
    return correct_spike && correct_heartbeat;
}

static inline void assert_valid_Message(struct Message * msg) {
#ifndef NDEBUG
    switch (msg->type) {
        case MESSAGE_TYPE_heartbeat:
            assert(msg->fired ? msg->rng_count >= 0 : msg->rng_count == 0);
            break;
        case MESSAGE_TYPE_spike:
            assert(!isnan(msg->spike_current));
            assert(0 <= msg->neuron_from);
            assert(0 <= msg->neuron_to);
            assert((uint64_t) msg->neuron_from < g_tw_total_lps);
            assert((uint64_t) msg->neuron_to < g_tw_total_lps);
            assert(0 <= msg->neuron_from_gid);
            assert(0 <= msg->neuron_to_gid);
            break;
    }
#endif // NDEBUG
}

#endif /* end of include guard */

#ifndef DORYTA_EVENT_H
#define DORYTA_EVENT_H

#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <stddef.h>

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
 */
struct Message {
  enum MESSAGE_TYPE type;
  double time_processed;
  union {
    struct { // message type = heartbeat
      bool fired;
    };
    struct { // message type = spike
      float spike_current;
    };
  };
  // This reserves MESSAGE_SIZE_REVERSE bytes (enough for
  // MESSAGE_SIZE_REVERSE/4 double-precision floating point numbers,
  // MESSAGE_SIZE_REVERSE/8 64-bit integers, or really anything that can occupy
  // that same space. This is meant to be used by the neuron mechanism to store
  // and restore the state of the neuron.)
  char reserved_for_reverse[MESSAGE_SIZE_REVERSE];
};

#define MESSAGE_SIZE ()

static inline void initialize_Message(struct Message * msg, enum MESSAGE_TYPE type) {
    msg->time_processed = -1;
    switch (type) {
        case MESSAGE_TYPE_heartbeat:
            msg->type = MESSAGE_TYPE_heartbeat;
            msg->fired = false;
            break;
        case MESSAGE_TYPE_spike:
            msg->type = MESSAGE_TYPE_spike;
            msg->spike_current = 1;
            break;
    }
}

static inline bool is_valid_Message(struct Message * msg) {
    bool const correct_current = msg->type == MESSAGE_TYPE_spike ?
                                 !isnan(msg->spike_current) : true;
    return correct_current;
}

static inline void assert_valid_Message(struct Message * msg) {
#ifndef NDEBUG
    if (msg->type == MESSAGE_TYPE_spike) {
        assert(!isnan(msg->spike_current));
    }
#endif // NDEBUG
}

#endif /* end of include guard */

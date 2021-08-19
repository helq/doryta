#ifndef DORYTA_EVENT_H
#define DORYTA_EVENT_H

#include <stdbool.h>
#include <assert.h>
#include <math.h>

// Order of types is important. An array of spikes is encoded as
// zeroed-terminated array which is only possible if the `spike`
// type is not zero
enum MESSAGE_TYPE {
  MESSAGE_TYPE_heartbeat,
  MESSAGE_TYPE_spike,
  MESSAGE_TYPE_fire,
};

/**
 * Invariants:
 * - `time_processed` is either -1 or any non-negative number
 * - `current` must be a number (not NaN)
 * - `fired` is true only if `time_processed` is not zero
 * - if `type` is fire, `spike_time_arrival` is non-negative and is smaller than
 *   `time_processed` (when this is not -1)
 */
struct Message {
  enum MESSAGE_TYPE type;
  double time_processed;
  bool fired;
  union {
    struct { // message type = spike
      float current;
    };
    struct { // message type = fire
      double spike_time_arrival;
    };
  };
};

static inline void initialize_Message(struct Message * msg) {
    msg->fired = false;
    msg->time_processed = -1;
}

static inline bool is_valid_Message(struct Message * msg) {
    bool const correct_time =
        (msg->time_processed == -1
        || msg->time_processed >= 0)
        && !isnan(msg->time_processed)
        && !isinf(msg->time_processed);
    bool const restriction_time_fired = (msg->fired == false)
                                      == (msg->time_processed == -1);
    bool const fired_is_bool = msg->fired == true || msg->fired == false;
    bool const correct_current = msg->type == MESSAGE_TYPE_spike ?
                                 !isnan(msg->current) : true;
    bool correct_spike_time = true;
    if (msg->type == MESSAGE_TYPE_fire) {
        if (isnan(msg->spike_time_arrival) || isinf(msg->spike_time_arrival)) {
            return false;
        }
        if (msg->time_processed != -1 && correct_time) {
            correct_spike_time = msg->spike_time_arrival < msg->time_processed;
        }
    }
    return correct_time && correct_current && restriction_time_fired
        && fired_is_bool && correct_spike_time;
}

static inline void assert_valid_Message(struct Message * msg) {
#ifndef NDEBUG
    assert(msg->time_processed == -1 || msg->time_processed >= 0);
    if (msg->type == MESSAGE_TYPE_spike) {
        assert(!isnan(msg->current));
    }
    assert((msg->fired == false) == (msg->time_processed == -1));
    assert(msg->fired == true || msg->fired == false);
    if (msg->type == MESSAGE_TYPE_fire) {
        assert(!isnan(msg->spike_time_arrival));
        assert(!isinf(msg->spike_time_arrival));
        if (msg->time_processed != -1) {
            assert(msg->spike_time_arrival < msg->time_processed);
        }
    }
#endif // NDEBUG
}

#endif /* end of include guard */

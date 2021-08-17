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
};

/**
 * Invariants:
 * - `time_processed` is either -1 or any non-negative number
 * - `current` must be a number (not NaN)
 * - `fired` is true only if `time_processed` is not zero
 */
struct Message {
  enum MESSAGE_TYPE type;
  double time_processed;
  bool fired;
  union {
    struct { // message type = spike
      float current;
    };
  };
};

static inline void initialize_Message(struct Message * msg) {
    msg->fired = false;
    msg->time_processed = -1;
}

static inline bool is_valid_Message(struct Message * msg) {
    bool const correct_time = msg->time_processed == -1
                            || msg->time_processed >= 0;
    bool const correct_current = msg->type == MESSAGE_TYPE_spike ?
                                 !isnan(msg->current) : true;
    bool const restriction_time_fired = (msg->fired == false)
                                      == (msg->time_processed == -1);
    bool const fired_is_bool = msg->fired == true || msg->fired == false;
    return correct_time && correct_current && restriction_time_fired && fired_is_bool;
}

static inline void assert_valid_Message(struct Message * msg) {
#ifndef NDEBUG
    assert(msg->time_processed == -1 || msg->time_processed >= 0);
    if (msg->type == MESSAGE_TYPE_spike) {
        assert(!isnan(msg->current));
    }
    assert((msg->fired == false) == (msg->time_processed == -1));
    assert(msg->fired == true || msg->fired == false);
#endif // NDEBUG
}

#endif /* end of include guard */

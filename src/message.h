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
 * - `current` must be a number (not NaN)
 */
struct Message {
  enum MESSAGE_TYPE type;
  double time_processed;
  union {
    struct { // message type = heartbeat
      bool fired;
    };
    struct { // message type = spike
      float current;
    };
  };
};

static inline void initialize_Message(struct Message * msg, enum MESSAGE_TYPE type) {
    msg->time_processed = -1;
    switch (type) {
        case MESSAGE_TYPE_heartbeat:
            msg->type = MESSAGE_TYPE_heartbeat;
            msg->fired = false;
            break;
        case MESSAGE_TYPE_spike:
            msg->type = MESSAGE_TYPE_spike;
            msg->current = 1;
            break;
    }
}

static inline bool is_valid_Message(struct Message * msg) {
    bool const correct_current = msg->type == MESSAGE_TYPE_spike ?
                                 !isnan(msg->current) : true;
    return correct_current;
}

static inline void assert_valid_Message(struct Message * msg) {
#ifndef NDEBUG
    if (msg->type == MESSAGE_TYPE_spike) {
        assert(!isnan(msg->current));
    }
#endif // NDEBUG
}

#endif /* end of include guard */

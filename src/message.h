#ifndef DORYTA_EVENT_H
#define DORYTA_EVENT_H

#include <stdbool.h>
#include <assert.h>

enum MESSAGE_TYPE {
  MESSAGE_TYPE_spike,
  MESSAGE_TYPE_heartbeat,
};

struct Message {
  enum MESSAGE_TYPE type;
  double time_processed; //< This value should be either -1 or anything >= 0.
  union {
    struct { // message type = spike
      float current;
    };
  };
};

static inline bool is_valid_Message(struct Message * msg) {
    return false;
}

static inline void assert_valid_Message(struct Message * msg) {
#ifndef NDEBUG
    assert(false);
#endif // NDEBUG
}

#endif /* end of include guard */

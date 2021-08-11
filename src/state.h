#ifndef HIGHLIFE_STATE_H
#define HIGHLIFE_STATE_H

/** @file
 * Definition of messages and LP states for this simulation.
 */

#include <ross.h>
#include <stdbool.h>
#include <assert.h>

/** The world for a HighLife simulation is composed of stacked mini-worlds, one
 * per LP. The size of each mini-world is `W_WIDTH x W_HEIGHT`.
 */
#define W_WIDTH 20
#define W_HEIGHT (20 + 2)

// ================================ State struct ===============================

/** This defines the state for all LPs.
 * Struct invariants:
 * - `steps >= 0`
 * - If `grid` is null, so has to be `fp`
 * - The size of `grid` is `W_WIDTH * W_HEIGHT` (This is
 *   impossible to check or enforce, please make sure to allocate
 *   the space correctly).
 * - The contents of all cells in `grid` (if not NULL) are either
 *   0 or 1
 **/
struct HighlifeState {
  int steps;            /**< Number of HighLife steps executed by LP. If a
                          mini-world stays the same after one step of HighLife
                          on its grid, it is not a step and it won't
                          alter/bother neighbour. */
  bool next_beat_sent;  /**< This boolean is used to determine whether the next
                          heartbeat. has already been produced by another event
                          or it should be created by the current event being
                          processed. */
  unsigned char *grid;  /**< Pointer to the mini-world (grid). */
  // TODO(helq): the state of an LP shouldn't include a file handler, this
  // puts, probably a lot of pression on IO, even more if it is parallel IO
  FILE *fp;             /**< The file in which to save some stats and the grid
                          at the start and end of simulation. */
};

static inline bool is_valid_HighlifeState(struct HighlifeState * hs) {
    bool invariants_1_to_2 = (hs->steps >= 0)
                         && ((hs->grid == NULL) == (hs->fp == NULL));

    if (invariants_1_to_2 && hs->grid != NULL) {
        // checking fourth invariant (opposite of invariant, actually)
        for (size_t i = 0; i < W_WIDTH * W_HEIGHT; i++) {
            // Detecting if number is different from 0 or 1
            if (hs->grid[i] & (~0x1)) {
                return false;
            }
        }
        return true;
    }
    return invariants_1_to_2;
}

static inline void assert_valid_HighlifeState(struct HighlifeState * hs) {
#ifndef NDEBUG
    // First invariant
    assert(hs->steps >= 0);
    // Second invariant
    assert((hs->grid == NULL) == (hs->fp == NULL));
    // checking fourth invariant
    if (hs->grid != NULL) {
        for (size_t i = 0; i < W_WIDTH * W_HEIGHT; i++) {
            assert(! (hs->grid[i] & (~0x1)));
        }
    }
#endif // NDEBUG
}

// ========================= Message enums and structs =========================

/** There are two types of messages. */
enum MESSAGE_TYPE {
  MESSAGE_TYPE_step,        /**< This is a heartbeat. It asks the LP to perform
                              a step of HighLife in its grid. */
  MESSAGE_TYPE_row_update,  /**< This tells the LP to replace its old row
                              (either top or bottom) for the row contained in
                              the body. */
};

/** A MESSAGE_TYPE_row_update message can be of two kinds. */
enum ROW_DIRECTION {
  ROW_DIRECTION_up_row,    /**< The row in the message should replace the top
                             row in the grid. */
  ROW_DIRECTION_down_row,  /**< The row in the message should replace the
                             bottom row in the grid. */
};

/** This contains all data sent in an event.
 * There is only one type of message, so we define all the space needed to
 * store any of the reversible states as well as the data for the new update.
 * Invariants:
 * - `type` can only be one of `MESSAGE_TYPE`
 * - If `type` is step and `rev_state` is a non-NULL
 *   pointer, then `rev_state` must contain a valid grid (composed
 *   of only 1s or 0s)
 * - If `type` is row_update, `dir` can only be one of `ROW_DIRECTION`
 * - If `type` is row_update, then `row` must contain only 0s or 1s.
 */
struct Message {
  enum MESSAGE_TYPE type;
  tw_lpid sender;              /**< Unique identifier of the LP who sent the
                                 message. */
  union {
    struct { // message type = step
      unsigned char *rev_state;    /**< Storing the previous state to be
                                     recovered by the reverse message handler.
                                     */
    };
    struct { // message type = row_update
      enum ROW_DIRECTION dir;      /**< In case `type` is ROW_UPDATE, this
                                     indicates the direction. */
      unsigned char row[W_WIDTH];  /**< In case `type` is ROW_UPDATE, this
                                     contains the new row values. */
    };
  };
};

static inline bool is_valid_Message(struct Message * msg) {
    if (msg->type != MESSAGE_TYPE_step
        && msg->type != MESSAGE_TYPE_row_update) {
        return false;
    }
    if (msg->type == MESSAGE_TYPE_step && msg->rev_state != NULL) {
        for (size_t i = 0; i < W_WIDTH * W_HEIGHT; i++) {
            if (msg->rev_state[i] & (~0x1)) {
                return false;
            }
        }
    }
    if (msg->type == MESSAGE_TYPE_row_update) {
        if (msg->dir != ROW_DIRECTION_down_row
            && msg->dir != ROW_DIRECTION_up_row) {
            return false;
        }
        for (size_t i = 0; i < sizeof(msg->row); i++) {
            if (msg->row[i] & (~0x1)) {
                return false;
            }
        }
    }
    return true;
}

// This function might fail if `msg->rev_state` doesn't point to a
// valid memory region
static inline void assert_valid_Message(struct Message * msg) {
#ifndef NDEBUG
    assert(msg->type == MESSAGE_TYPE_step
            || msg->type == MESSAGE_TYPE_row_update);
    if (msg->type == MESSAGE_TYPE_step && msg->rev_state != NULL) {
        for (size_t i = 0; i < W_WIDTH * W_HEIGHT; i++) {
            assert(! (msg->rev_state[i] & (~0x1)));
        }
    }
    if (msg->type == MESSAGE_TYPE_row_update) {
        assert(msg->dir == ROW_DIRECTION_down_row
                || msg->dir == ROW_DIRECTION_up_row);
        for (size_t i = 0; i < sizeof(msg->row); i++) {
            assert(! (msg->row[i] & (~0x1)));
        }
    }
#endif // NDEBUG
}

#endif /* end of include guard */

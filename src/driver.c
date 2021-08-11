#include "driver.h"
#include <stdbool.h>
#include <ross.h>
#include <stdio.h>

// ================================= Global variables ================================

static int init_pattern = 0;

void driver_config(int init_pattern_var) {
    init_pattern = init_pattern_var;
}


// ================================ Helper definitions ===============================

#define POINTER_SWAP(x, y) {void *tmp = x; x = y; y = tmp;}


// ================================= Helper functions ================================

static bool array_swap(unsigned char *grid, unsigned char *grid_msg, size_t n_cells) {
  bool change = false;
  unsigned char tmp;
  for (size_t i = 0; i < n_cells; i++, grid_msg++, grid++) {
    tmp = *grid_msg;
    *grid_msg = *grid;
    *grid = tmp;

    if(!change && (*grid != *grid_msg)) {
      change = true;
    }
  }
  return change;
}


static inline void copy_row(unsigned char * const into, unsigned char const * const from) {
  for (int i = 0; i < W_WIDTH; i++) {
    into[i] = from[i];
  }
}


// ------------------------- Helper initialization functions -------------------------

static inline void hl_init_all_zeros(struct HighlifeState *s) {
  for (size_t i = 0; i < W_WIDTH * W_HEIGHT; i++) {
      s->grid[i] = 0;
  }
}


static inline void hl_init_all_ones(struct HighlifeState *s) {
  hl_init_all_zeros(s);
  for (size_t i = W_WIDTH; i < W_WIDTH * (W_HEIGHT - 1); i++) {
      s->grid[i] = 1;
  }
}


static inline void hl_init_ones_in_middle(struct HighlifeState *s) {
  hl_init_all_zeros(s);
  // Iterating over row 10
  for (size_t i = 10 * W_WIDTH; i < 11 * W_WIDTH; i++) {
    if ((i >= (10 * W_WIDTH + 10)) && (i < (10 * W_WIDTH + 20))) {
        s->grid[i] = 1;
    }
  }
}


static inline void hl_init_ones_at_corners(struct HighlifeState *s, unsigned long self) {
  hl_init_all_zeros(s);

  if (self == 0) {
    s->grid[W_WIDTH] = 1;                                   // upper left
    s->grid[2 * W_WIDTH - 1] = 1;                           // upper right
  }
  if (self == g_tw_total_lps - 1) {
    s->grid[(W_WIDTH * (W_HEIGHT - 2))] = 1;                // lower left
    s->grid[(W_WIDTH * (W_HEIGHT - 2)) + W_WIDTH - 1] = 1;  // lower right
  }
}


static inline void hl_init_spinner_at_corner(struct HighlifeState *s, unsigned long self) {
  hl_init_all_zeros(s);

  if (self == 0) {
    s->grid[W_WIDTH] = 1;            // upper left
    s->grid[W_WIDTH+1] = 1;            // upper left +1
    s->grid[2*W_WIDTH - 1] = 1;  // upper right
  }
}


static inline void hl_init_replicator(struct HighlifeState *s, unsigned long self) {
  hl_init_all_zeros(s);

  if (self == 0) {
    size_t x, y;
    x = W_WIDTH / 2;
    y = W_HEIGHT / 2;

    s->grid[x + y * W_WIDTH + 1] = 1;
    s->grid[x + y * W_WIDTH + 2] = 1;
    s->grid[x + y * W_WIDTH + 3] = 1;
    s->grid[x + (y + 1) * W_WIDTH] = 1;
    s->grid[x + (y + 2) * W_WIDTH] = 1;
    s->grid[x + (y + 3) * W_WIDTH] = 1;
  }
}


static inline void hl_init_diagonal(struct HighlifeState *s) {
  hl_init_all_zeros(s);

  for (int i = 0; i < W_WIDTH && i < W_HEIGHT; i++) {
      s->grid[(i+1)*W_WIDTH + i] = 1;
  }
}


static void hl_print_world(FILE *stream, struct HighlifeState *s) {
  size_t i, j;

  fprintf(stream, "Print World - Iteration %d\n", s->steps);

  fprintf(stream, "Ghost:  ");
  for (j = 0; j < W_WIDTH; j++) {
    fprintf(stream, "%u ", (unsigned int)s->grid[j]);
  }
  fprintf(stream, "\n");

  for (i = 1; i < W_HEIGHT-1; i++) {
    fprintf(stream, "Row %2zu: ", i);
    for (j = 0; j < W_WIDTH; j++) {
      fprintf(stream, "%u ", (unsigned int)s->grid[(i * W_WIDTH) + j]);
    }
    fprintf(stream, "\n");
  }

  fprintf(stream, "Ghost:  ");
  for (j = 0; j < W_WIDTH; j++) {
    fprintf(stream, "%u ", (unsigned int)s->grid[(i * W_WIDTH) + j]);
  }
  fprintf(stream, "\n");
}


// -------------------------- Helper HighLife step functions -------------------------

/** Determines the number of alive cells around a given point in space */
static inline unsigned int hl_count_alive_cells(
        unsigned char *data, size_t x0, size_t x1, size_t x2, size_t y0, size_t y1, size_t y2) {
  // y1+x1 represents the position in the one dimensional array where
  // the point (y, x) would be located in 2D space, if the array was a
  // two dimensional array.
  // y1+x0 is the position to the left of (y, x)
  // y0+x1 is the position on top of (y, x)
  // y2+x2 is the position diagonally bottom, right to (y, x)
  return data[y0 + x0] + data[y0 + x1] + data[y0 + x2]
       + data[y1 + x0]                 + data[y1 + x2]
       + data[y2 + x0] + data[y2 + x1] + data[y2 + x2];
}


/** Serial version of standard byte-per-cell life */
static int hl_iterate_serial(unsigned char *grid, unsigned char *grid_msg) {
  int changed = 0; // Tracking change in the grid

  // The code seems more intricate than what it actually is.
  // The second and third loops are in charge of computing a single
  // transition of the game of life.
  // The first loop runs the transition a number of times (`iterations`
  // times).
  for (size_t y = 1; y < W_HEIGHT-1; ++y) {
    // This seems oddly complicated, but it is just obfuscated module
    // operations. The grid of the game of life fold on itself. It is
    // geometrically a donut/torus.
    //
    // `y1` is the position in the 1D array which corresponds to the
    // first column for the row `y`
    //
    // `x1` is the relative position of `x` with respect to the first
    // column in row `y`
    //
    // For example, let's assume that (y, x) = (2, 3) and the following
    // world:
    //
    //   0 1 2 3 4
    // 0 . . . . .
    // 1 . . . . .
    // 2 . . . # .
    // 3 . . . . .
    //
    // Then x1 = x = 3,
    // and  y1 = y * 5 = 10.
    // Which, when added up, give us the position in the 1D array
    //
    // . . . . . . . . . . . . . # . . . . . .
    // 0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1 1 1 1 1
    //                     0 1 2 3 4 5 6 7 8 9
    //
    // x0 is the column to the left of x1 (taking into account the torus geometry)
    // y0 is the row above to y1 (taking into account the torus geometry)
    size_t y0, y1, y2;
    y1 = y * W_WIDTH;
    y0 = ((y + W_HEIGHT - 1) % W_HEIGHT) * W_WIDTH;
    y2 = ((y + 1) % W_HEIGHT) * W_WIDTH;

    for (size_t x = 0; x < W_WIDTH; ++x) {
      // Computing important positions for x
      size_t x0, x1, x2;
      x1 = x;
      x0 = (x1 + W_WIDTH - 1) % W_WIDTH;
      x2 = (x1 + 1) % W_WIDTH;

      // Determining the next state for a single cell. We count how
      // many alive neighbours the current (y, x) cell has and
      // follow the rules for HighLife
      int neighbours = hl_count_alive_cells(grid, x0, x1, x2, y0, y1, y2);
      if (grid[y1 + x1]) {  // if alive
        grid_msg[y1 + x1] = neighbours == 2 || neighbours == 3;
      } else {  // if dead
        grid_msg[y1 + x1] = neighbours == 3 || neighbours == 6;
      }

      if ((!changed) && (grid[y1 + x1] != grid_msg[y1 + x1])) {
          changed = 1;
      }
    }
  }

  return changed;
}


/** Sends a heartbeat */
static void send_tick(tw_lp *lp, float dt) {
  uint64_t const self = lp->gid;
  tw_event *e = tw_event_new(self, dt, lp);
  struct Message *msg = tw_event_data(e);
  msg->type = MESSAGE_TYPE_step;
  msg->sender = self;
  msg->rev_state = NULL;
  tw_event_send(e);
  assert_valid_Message(msg);
}


/** Sends a (new) rows to neighboring grids/LPs/mini-worlds */
static void send_rows(struct HighlifeState *s, tw_lp *lp) {
  uint64_t const self = lp->gid;

  // Sending rows to update
  int lp_id_up = (self + g_tw_total_lps - 1) % g_tw_total_lps;
  int lp_id_down = (self+1) % g_tw_total_lps;

  tw_event *e_drow = tw_event_new(lp_id_up, 0.5, lp);
  struct Message *msg_drow = tw_event_data(e_drow);
  msg_drow->type = MESSAGE_TYPE_row_update;
  msg_drow->dir = ROW_DIRECTION_down_row; // Other LP's down row, not mine
  copy_row(msg_drow->row, s->grid + W_WIDTH);
  tw_event_send(e_drow);
  assert_valid_Message(msg_drow);

  tw_event *e_urow = tw_event_new(lp_id_down, 0.5, lp);
  struct Message *msg_urow = tw_event_data(e_urow);
  msg_urow->type = MESSAGE_TYPE_row_update;
  msg_urow->dir = ROW_DIRECTION_up_row;
  copy_row(msg_urow->row, s->grid + (W_WIDTH * (W_HEIGHT - 2)));
  tw_event_send(e_urow);
  assert_valid_Message(msg_drow);
}


// ========================== Helper HighLife step functions =========================
// These functions are called directly by ROSS

// LP initialization. Called once for each LP
void highlife_init(struct HighlifeState *s, struct tw_lp *lp) {
  uint64_t const self = lp->gid;
  s->grid = malloc(W_WIDTH * W_HEIGHT * sizeof(unsigned char));

  switch (init_pattern) {
  case 0: hl_init_all_zeros(s); break;
  case 1: hl_init_all_ones(s); break;
  case 2: hl_init_ones_in_middle(s); break;
  case 3: hl_init_ones_at_corners(s, self); break;
  case 4: hl_init_spinner_at_corner(s, self); break;
  case 5: hl_init_replicator(s, self); break;
  case 6: hl_init_diagonal(s); break;
  default:
    printf("Pattern %u has not been implemented \n", init_pattern);
    /*exit(-1);*/
    MPI_Abort(MPI_COMM_WORLD, -1);
  }

  // Finding name for file
  char const fmt[] = "output/highlife-gid=%lu.txt";
  int sz = snprintf(NULL, 0, fmt, self);
  char filename[sz + 1]; // `+ 1` for terminating null byte
  snprintf(filename, sizeof(filename), fmt, self);

  // Creating file handler and printing initial state of the grid
  s->fp = fopen(filename, "w");
  if (!s->fp) {
    fprintf(stderr, "File opening failed: '%s'\n", filename);
    MPI_Abort(MPI_COMM_WORLD, -1);
  } else {
    hl_print_world(s->fp, s);
    fprintf(s->fp, "\n");
  }

  // Tick message to myself
  send_tick(lp, 1);
  s->next_beat_sent = 1;
  // Sending rows to update
  send_rows(s, lp);
  assert_valid_HighlifeState(s);
}


// Forward event handler
void highlife_event(
        struct HighlifeState *s,
        struct tw_bf *bitfield,
        struct Message *in_msg,
        struct tw_lp *lp) {
  // initialize the bit field
  bitfield->c0 = s->next_beat_sent;

  // handle the message
  switch (in_msg->type) {
  case MESSAGE_TYPE_step:
    in_msg->rev_state = calloc(W_WIDTH * W_HEIGHT, sizeof(unsigned char));

    // Next step in the simulation (is stored in second parameter)
    int changed = hl_iterate_serial(s->grid, in_msg->rev_state);
    // Exchanging parameters from one site to the other
    POINTER_SWAP(s->grid, in_msg->rev_state);
    s->next_beat_sent = 0;

    s->steps++;
    if (changed) {
      // Sending tick for next STEP
      send_tick(lp, 1);
      // Sending rows to update
      send_rows(s, lp);
      s->next_beat_sent = 1;
    }
    break;

  case MESSAGE_TYPE_row_update:
    {
    bool change;  //< To store whether the update took place or there was no change between
                  // the two rows
    switch (in_msg->dir) {
    case ROW_DIRECTION_up_row:
      change = array_swap(s->grid, in_msg->row, W_WIDTH);
      break;
    case ROW_DIRECTION_down_row:
      change = array_swap(s->grid + W_WIDTH*(W_HEIGHT-1), in_msg->row, W_WIDTH);
      /*hl_print_world(stdout, s);*/
      break;
    }
    if (!s->next_beat_sent && change) {
      // In case the heartbeat hasn't been sent yet and this message modified the state of
      // the grid, then sent a heartbeat to self
      send_tick(lp, 0.5);
      s->next_beat_sent = 1;
    }
    break;
    }
  }
  assert_valid_Message(in_msg);
}


// Reverse Event Handler
// Notice that all operations are reversed using the data stored in either the reverse
// message or the bit field
void highlife_event_reverse(
        struct HighlifeState *s,
        struct tw_bf *bitfield,
        struct Message *in_msg,
        struct tw_lp *lp) {
  (void)lp;

  // handle the message
  switch (in_msg->type) {
  case MESSAGE_TYPE_step:
    s->steps--;
    POINTER_SWAP(s->grid, in_msg->rev_state);
    free(in_msg->rev_state);  // Freeing memory allocated by forward handler
    in_msg->rev_state = NULL;
    break;
  case MESSAGE_TYPE_row_update:
    switch (in_msg->dir) {
    case ROW_DIRECTION_up_row:
      array_swap(s->grid, in_msg->row, W_WIDTH);
      break;
    case ROW_DIRECTION_down_row:
      array_swap(s->grid + W_WIDTH*(W_HEIGHT-1), in_msg->row, W_WIDTH);
      break;
    }
    break;
  }

  s->next_beat_sent = bitfield->c0;
  assert_valid_Message(in_msg);
}


// Commit event handler
// This function is only called when it can be make sure that the message won't be
// roll back. Either the commit or reverse handler will be called, not both
void highlife_event_commit(
        struct HighlifeState *s,
        struct tw_bf *bitfield,
        struct Message *in_msg,
        struct tw_lp *lp) {
  (void)s;
  (void)bitfield;
  (void)lp;

  // handle the message
  if (in_msg->type == MESSAGE_TYPE_step) {
    free(in_msg->rev_state);  // Freeing memory allocated by forward handler
    in_msg->rev_state = NULL;
  }
  assert_valid_Message(in_msg);
}


// The finalization function
// Reporting any final statistics for this LP in the file previously opened
void highlife_final(struct HighlifeState *s, struct tw_lp *lp) {
  uint64_t const self = lp->gid;
  printf("LP %lu handled %d MESSAGE_TYPE_step messages\n", self, s->steps);
  printf("LP %lu: The current (local) time is %f\n\n", self, tw_now(lp));

  if (!s->fp) {
    fprintf(stderr, "File to output is closed!\n");
    MPI_Abort(MPI_COMM_WORLD, -1);
  } else {
    fprintf(s->fp, "%lu handled %d MESSAGE_TYPE_step messages\n", self, s->steps);
    fprintf(s->fp, "The current (local) time is %f\n\n", tw_now(lp));
    hl_print_world(s->fp, s);
    fclose(s->fp);
  }
  free(s->grid);
}

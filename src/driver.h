#ifndef DORYTA_DRIVER_H
#define DORYTA_DRIVER_H

/** @file
 * Functions implementing a neuron as a discrete event simulator
 */

#include "state.h"

/** Setting global variables to by the simulation. */
void driver_config(int init_pattern);

/** Grid initialization and first heartbeat. */
void highlife_init(struct HighlifeState *s, struct tw_lp *lp);

/** Forward event handler. */
void highlife_event(
        struct HighlifeState *s,
        struct tw_bf *bf,
        struct Message *in_msg,
        struct tw_lp *lp);

/** Reverse event handler. */
void highlife_event_reverse(
        struct HighlifeState *s,
        struct tw_bf *bf,
        struct Message *in_msg,
        struct tw_lp *lp);

/** Commit event handler. */
void highlife_event_commit(
        struct HighlifeState *s,
        struct tw_bf *bf,
        struct Message *in_msg,
        struct tw_lp *lp);

/** Cleaning and printing info before shut down. */
void highlife_final(struct HighlifeState *s, struct tw_lp *lp);

#endif /* end of include guard */

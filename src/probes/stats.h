#ifndef DORYTA_PROBES_STATS_H
#define DORYTA_PROBES_STATS_H

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct NeuronLP;
struct Message;
struct tw_lp;

/** This probe records the number of leak, integrate and fire operations.
 */
void probes_stats_init(size_t neurons_in_pe, char const output_path[],
                       char const filename[]);

void probes_stats_record(struct NeuronLP *, struct Message *, struct tw_lp *);

void probes_stats_deinit(void);

#endif /* end of include guard */

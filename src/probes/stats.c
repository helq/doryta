#include "stats.h"
#include "../driver/neuron.h"
#include "ross.h"

#include <stdio.h>

struct NeuronLPStats {
    int32_t neuron;
    int64_t leaks;
    int64_t integrations;
    int64_t fires;
};

static struct NeuronLPStats * stats = NULL;
static size_t neurons_in_pe;
static char const * output_path = NULL;
static char const * filename = NULL;

void probes_stats_init(size_t neurons_in_pe_, char const output_path_[],
                       char const filename_[]) {
    neurons_in_pe = neurons_in_pe_;
    output_path = output_path_;
    filename = filename_;
    stats = calloc(neurons_in_pe, sizeof(struct NeuronLPStats));
    for (size_t i = 0; i < neurons_in_pe; i++) {
        stats[i].neuron = -1;
    }
}


void probes_stats_record(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    assert(stats != NULL);
    // Setting up neuron ID
    if (stats[lp->id].neuron == -1) {
        stats[lp->id].neuron = neuronLP->doryta_id;
    }

    switch (msg->type) {
        case MESSAGE_TYPE_heartbeat:
            stats[lp->id].leaks++;
            if (msg->fired) {
                stats[lp->id].fires++;
            }
            break;
        case MESSAGE_TYPE_spike:
            stats[lp->id].integrations++;
            break;
    }
}


static void stats_save(void) {
    assert(output_path != NULL);
    assert(filename != NULL);
    unsigned long self = g_tw_mynode;

    // Finding name for file
    char const fmt[] = "%s/%s-stats-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, output_path, filename, self);
    char filename_path[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename_path, sizeof(filename_path), fmt, output_path, filename, self);

    FILE * fp = fopen(filename_path, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < neurons_in_pe; i++) {
            if (stats[i].neuron == -1) {
                continue;
            }
            fprintf(fp, "%" PRIi32 "\t%" PRIi64 "\t%" PRIi64 "\t%" PRIi64 "\n",
                    stats[i].neuron,
                    stats[i].leaks,
                    stats[i].integrations,
                    stats[i].fires);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store `spikes` in file %s\n", filename_path);
    }
}


void probes_stats_deinit(void) {
    assert(stats != NULL);
    stats_save();
    free(stats);
}

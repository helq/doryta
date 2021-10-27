#include "firing.h"
#include "../message.h"
#include "../storable_spikes.h"
#include "../driver/neuron_lp.h"
#include "ross.h"

#include <stdio.h>

static struct StorableSpike * firing_spikes = NULL;
static size_t buffer_size;
static size_t buffer_used = 0;
static bool buffer_limit_hit = false;
static char const * output_path = NULL;

void probes_firing_init(size_t buffer_size_, char const output_path_[]) {
    buffer_size = buffer_size_;
    output_path = output_path_;
    firing_spikes = malloc(buffer_size * sizeof(struct StorableSpike));
}


void probes_firing_record(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    (void) lp;
    assert(firing_spikes != NULL);

    if (msg->type == MESSAGE_TYPE_heartbeat && msg->fired) {
        if (buffer_used < buffer_size) {
            firing_spikes[buffer_used].neuron    = neuronLP->doryta_id;
            firing_spikes[buffer_used].time      = msg->time_processed;
            firing_spikes[buffer_used].intensity = 1;
            buffer_used++;
        } else {
            buffer_limit_hit = true;
        }
    }
}


static void firing_save(void) {
    assert(output_path != NULL);
    unsigned long self = g_tw_mynode;
    if (buffer_limit_hit) {
        fprintf(stderr, "Only the first %ld `spikes` have been recorded\n", buffer_size);
    }

    // Finding name for file
    char const fmt[] = "%s-spikes-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, output_path, self);
    char filename[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename, sizeof(filename), fmt, output_path, self);

    FILE * fp = fopen(filename, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < buffer_used; i++) {
            fprintf(fp, "%" PRIu64 "\t%f\n", firing_spikes[i].neuron, firing_spikes[i].time);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store `spikes` in file %s\n", filename);
    }
}


void probes_firing_deinit(void) {
    assert(firing_spikes != NULL);
    firing_save();
    free(firing_spikes);
}

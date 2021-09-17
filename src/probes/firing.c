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

void initialize_record_firing(size_t buffer_size_) {
    buffer_size = buffer_size_;
    firing_spikes = malloc(buffer_size * sizeof(struct StorableSpike));
}


void record_firing(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    (void) neuronLP;
    assert(firing_spikes != NULL);

    if (msg->type == MESSAGE_TYPE_heartbeat && msg->fired) {
        if (buffer_used < buffer_size) {
            firing_spikes[buffer_used].neuron    = lp->id;
            firing_spikes[buffer_used].time      = msg->time_processed;
            firing_spikes[buffer_used].intensity = 1;
            buffer_used++;
        } else {
            buffer_limit_hit = true;
        }
    }
}


void save_record_firing(char const * path) {
    assert(firing_spikes != NULL);
    unsigned long self = g_tw_mynode;
    if (buffer_limit_hit) {
        fprintf(stderr, "Only the first %ld spikes have been recorded\n", buffer_size);
    }

    // Finding name for file
    char const fmt[] = "%s-spikes-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, path, self);
    char filename[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename, sizeof(filename), fmt, path, self);

    FILE * fp = fopen(filename, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < buffer_used; i++) {
            fprintf(fp, "%lu\t%f\n", firing_spikes[i].neuron, firing_spikes[i].time);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store spikes in file %s\n", filename);
    }
}


void deinitialize_record_firing(void) {
    assert(firing_spikes != NULL);
    free(firing_spikes);
}

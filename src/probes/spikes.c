#include "spikes.h"
#include "../mapping.h"
#include "../message.h"
#include "../driver/lp_neuron.h"
#include "ross.h"

#include <stdio.h>

struct SpikeToStore {
    uint64_t neuron;
    double time;
};

static struct SpikeToStore * spikes = NULL;
static size_t buffer_size;
static size_t buffer_used = 0;
static bool buffer_limit_hit = false;

void initialize_record_spikes(size_t buffer_size_) {
    buffer_size = buffer_size_;
    spikes = malloc(buffer_size * sizeof(struct SpikeToStore));
}


void record_spikes(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    (void) neuronLP;
    assert(spikes != NULL);
    if(buffer_used < buffer_size) {
        spikes[buffer_used].neuron = get_neuron_id(lp);
        spikes[buffer_used].time   = msg->time_processed;
        buffer_used++;
    } else {
        buffer_limit_hit = true;
    }
}


void save_record_spikes(char const * path) {
    assert(spikes != NULL);
    unsigned long self = g_tw_mynode;
    if (buffer_limit_hit) {
        fprintf(stderr, "Only the first %ld spikes have been recorded\n", buffer_size);
    }

    // Finding name for file
    char const fmt[] = "%s-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, path, self);
    char filename[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename, sizeof(filename), fmt, path, self);

    FILE * fp = fopen(filename, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < buffer_used; i++) {
            fprintf(fp, "%lu\t%f\n", spikes[i].neuron, spikes[i].time);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store spikes in file %s\n", filename);
    }
}


void deinitialize_record_spikes(void) {
    assert(spikes != NULL);
    free(spikes);
}

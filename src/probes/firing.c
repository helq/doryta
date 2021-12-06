#include "firing.h"
#include "../storable_spikes.h"
#include "../driver/neuron.h"
#include "ross.h"

#include <stdio.h>

static struct StorableSpike * firing_spikes = NULL;
static size_t buffer_size;
static size_t buffer_used = 0;
static bool buffer_limit_hit = false;
static char const * output_path = NULL;
static char const * filename = NULL;
static bool only_output_neurons;

void probes_firing_init(
        size_t buffer_size_,
        char const output_path_[],
        char const filename_[],
        bool only_output_neurons_
        ) {
    buffer_size = buffer_size_;
    output_path = output_path_;
    filename = filename_;
    only_output_neurons = only_output_neurons_;
    firing_spikes = malloc(buffer_size * sizeof(struct StorableSpike));
}


void probes_firing_record(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    (void) lp;
    assert(firing_spikes != NULL);

    bool const was_fired = msg->type == MESSAGE_TYPE_heartbeat && msg->fired;
    bool const record_neuron = only_output_neurons ? neuronLP->to_contact.num == 0 : true;

    if (was_fired && record_neuron) {
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
    assert(filename != NULL);
    unsigned long self = g_tw_mynode;
    if (buffer_limit_hit) {
        fprintf(stderr, "Only the first %ld `spikes` have been recorded\n", buffer_size);
    }

    // Finding name for file
    char const fmt[] = "%s/%s-spikes-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, output_path, filename, self);
    char filename_path[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename_path, sizeof(filename_path), fmt, output_path, filename, self);

    FILE * fp = fopen(filename_path, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < buffer_used; i++) {
            fprintf(fp, "%" PRIi32 "\t%f\n", firing_spikes[i].neuron, firing_spikes[i].time);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store `spikes` in file %s\n", filename_path);
    }
}


void probes_firing_deinit(void) {
    assert(firing_spikes != NULL);
    firing_save();
    free(firing_spikes);
}

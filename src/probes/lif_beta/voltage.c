#include "voltage.h"
#include "../../message.h"
#include "../../driver/lp_neuron.h"
#include "../../neurons/lif_beta.h"
#include "ross.h"

#include <stdio.h>

struct StorableVoltage {
    uint64_t neuron;
    double time;
    float voltage;
};

static struct StorableVoltage * spikes = NULL;
static size_t buffer_size;
static size_t buffer_used = 0;
static bool buffer_limit_hit = false;

void initialize_record_lif_beta_voltages(size_t buffer_size_) {
    buffer_size = buffer_size_;
    spikes = malloc(buffer_size * sizeof(struct StorableVoltage));
}


void record_lif_beta_voltages(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    (void) lp;
    assert_valid_NeuronLP(neuronLP);
    assert(spikes != NULL);
    struct StorageInMessage * storage =
        (struct StorageInMessage *) msg->reserved_for_reverse;

    if (msg->type == MESSAGE_TYPE_heartbeat) {
        if (buffer_used < buffer_size) {
            spikes[buffer_used].neuron  = neuronLP->id;
            spikes[buffer_used].time    = msg->time_processed;
            spikes[buffer_used].voltage = storage->potential;
            buffer_used++;
        } else {
            buffer_limit_hit = true;
        }
    }
}


void save_record_lif_beta_voltages(char const * path) {
    assert(spikes != NULL);
    unsigned long self = g_tw_mynode;
    if (buffer_limit_hit) {
        fprintf(stderr, "Only the first %ld spikes have been recorded\n", buffer_size);
    }

    // Finding name for file
    char const fmt[] = "%s-voltage-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, path, self);
    char filename[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename, sizeof(filename), fmt, path, self);

    FILE * fp = fopen(filename, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < buffer_used; i++) {
            fprintf(fp, "%lu\t%f\t%f\n", spikes[i].neuron, spikes[i].time, spikes[i].voltage);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store spikes in file %s\n", filename);
    }
}


void deinitialize_record_lif_beta_voltages(void) {
    assert(spikes != NULL);
    free(spikes);
}

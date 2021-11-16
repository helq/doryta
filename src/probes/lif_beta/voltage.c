#include "voltage.h"
#include "../../driver/neuron_lp.h"
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
static char const * output_path = NULL;

void probes_lif_beta_voltages_init(size_t buffer_size_, char const output_path_[]) {
    buffer_size = buffer_size_;
    output_path = output_path_;
    spikes = malloc(buffer_size * sizeof(struct StorableVoltage));
}


void probes_lif_beta_voltages_record(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    (void) lp;
    assert_valid_NeuronLP(neuronLP);
    assert(spikes != NULL);
    struct StorageInMessageLifBeta * storage =
        (struct StorageInMessageLifBeta *) msg->reserved_for_reverse;

    if (msg->type == MESSAGE_TYPE_heartbeat) {
        if (buffer_used < buffer_size) {
            spikes[buffer_used].neuron  = neuronLP->doryta_id;
            spikes[buffer_used].time    = msg->time_processed;
            spikes[buffer_used].voltage = storage->potential;
            buffer_used++;
        } else {
            buffer_limit_hit = true;
        }
    }
}


static void voltages_save(void) {
    assert(output_path != NULL);
    unsigned long self = g_tw_mynode;
    if (buffer_limit_hit) {
        fprintf(stderr, "Only the first %ld `voltages` have been recorded\n", buffer_size);
    }

    // Finding name for file
    char const fmt[] = "%s-voltage-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, output_path, self);
    char filename[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename, sizeof(filename), fmt, output_path, self);

    FILE * fp = fopen(filename, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < buffer_used; i++) {
            fprintf(fp, "%" PRIu64 "\t%f\t%f\n", spikes[i].neuron, spikes[i].time, spikes[i].voltage);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store `voltages` in file %s\n", filename);
    }
}


void probes_lif_beta_voltages_deinit(void) {
    assert(spikes != NULL);
    voltages_save();
    free(spikes);
}

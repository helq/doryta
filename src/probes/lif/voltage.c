#include "voltage.h"
#include "../../message.h"
#include "../../driver/neuron_lp.h"
#include "../../neurons/lif.h"
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
static char const * filename = NULL;

void probes_lif_voltages_init(
        size_t buffer_size_,
        char const output_path_[],
        char const filename_[]) {
    buffer_size = buffer_size_;
    output_path = output_path_;
    filename = filename_;
    spikes = malloc(buffer_size * sizeof(struct StorableVoltage));
}


void probes_lif_voltages_record(
        struct NeuronLP * neuronLP,
        struct Message * msg,
        struct tw_lp * lp) {
    (void) lp;
    assert_valid_NeuronLP(neuronLP);
    assert(spikes != NULL);
    struct StorageInMessageLif * storage =
        (struct StorageInMessageLif *) msg->reserved_for_reverse;

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
    assert(filename != NULL);
    unsigned long self = g_tw_mynode;
    if (buffer_limit_hit) {
        fprintf(stderr, "Only the first %ld `voltages` have been recorded\n", buffer_size);
    }

    // Finding name for file
    char const fmt[] = "%s/%s-voltage-gid=%lu.txt";
    int sz = snprintf(NULL, 0, fmt, output_path, filename, self);
    char filename_path[sz + 1]; // `+ 1` for terminating null byte
    snprintf(filename_path, sizeof(filename_path), fmt, output_path, filename, self);

    FILE * fp = fopen(filename_path, "w");

    if (fp != NULL) {
        for (size_t i = 0; i < buffer_used; i++) {
            fprintf(fp, "%" PRIu64 "\t%f\t%f\n", spikes[i].neuron, spikes[i].time, spikes[i].voltage);
        }

        fclose(fp);
    } else {
        fprintf(stderr, "Unable to store `voltages` in file %s\n", filename_path);
    }
}


void probes_lif_voltages_deinit(void) {
    assert(spikes != NULL);
    voltages_save();
    free(spikes);
}

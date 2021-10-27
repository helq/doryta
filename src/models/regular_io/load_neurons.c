#include "load_neurons.h"
#include <arpa/inet.h>
#include <pcg_basic.h>
#include <ross.h>
#include "../../driver/neuron_lp.h"
#include "../../layout/master.h"
#include "../../layout/standard_layouts.h"
#include "../../neurons/lif.h"
#include "../../storable_spikes.h"


static struct StorableSpike **spikes = NULL;


// Each one of them can print an error
static inline uint8_t  load_uint8(FILE *fp) {
    int res = fgetc(fp);
    if (res == EOF) {
        tw_error(TW_LOC, "Input file corrupt (note: premature EOF)");
    }
    return res;
}
/* Checks if the data was correctly read. If it didn't it fires a "ross" error.
 * `ret_code` is the value returned when executing `fread`, and `count` is
 * the expected number of objects to have read, which must be equal to `ret_code`.
 */
static inline void check_if_failure(FILE * fp, size_t ret_code, size_t count) {
    if (ret_code != count) { // Failure on reading file
        char const * error_msg;
        if (feof(fp)) {
            error_msg = "Input file corrupt (note: premature EOF)";
        } else if (ferror(fp)) {
            error_msg = "Input file corrupt (note: error found while reading file)";
        } else {
            error_msg = "Unknown error when reading file";
        }
        tw_error(TW_LOC, error_msg);
    }
}
static inline uint16_t load_uint16(FILE * fp) {
    uint16_t res;
    size_t ret_code = fread(&res, sizeof(res), 1, fp);
    check_if_failure(fp, ret_code, 1);
    return ntohs(res);
}
static inline uint32_t load_uint32(FILE * fp) {
    uint32_t res;
    size_t ret_code = fread(&res, sizeof(res), 1, fp);
    check_if_failure(fp, ret_code, 1);
    return ntohl(res);
}
static inline int32_t load_int32(FILE * fp) {
    uint32_t res;
    size_t ret_code = fread(&res, sizeof(res), 1, fp);
    check_if_failure(fp, ret_code, 1);
    return ntohl(res);
}
static inline float load_float(FILE * fp) {
    union {
        uint32_t ui32;
        float flt;
    } res;
    size_t ret_code = fread(&res.ui32, sizeof(res.ui32), 1, fp);
    check_if_failure(fp, ret_code, 1);
    res.ui32 = ntohl(res.ui32);
    return res.flt;
}
static inline void load_floats(FILE * fp, float * buffer, int32_t num) {
    size_t ret_code = fread(buffer, sizeof(float), num, fp);
    check_if_failure(fp, ret_code, num);
    for (int32_t i = 0; i < num; i++) {
        union {
            uint32_t ui32;
            float flt;
        } val;
        val.flt = buffer[i];
        val.ui32 = ntohl(val.ui32);
        buffer[i] = val.flt;
    }
}


struct ModelParams
model_load_neurons_init(struct SettingsNeuronLP * settings_neuron_lp,
        char const filename[]) {
    FILE * fp = fopen(filename, "rb");
    if (!fp) {
        tw_error(TW_LOC, "File `%s` could not be read", filename);
    }
    uint32_t magic = load_uint32(fp);
    if (magic != 0x23432BC4) {
        tw_error(TW_LOC, "Input file corrupt or unknown (note: incorrect magic number)");
    }
    uint16_t format = load_uint16(fp);
    if (format != 0x1) {
        tw_error(TW_LOC, "Input file corrupt or format unknown");
    }
    int32_t const total_num_neurons = load_int32(fp);
    uint8_t const neuron_groups = load_uint8(fp);
    uint8_t const synapse_groups = load_uint8(fp);
    float const beat = load_float(fp);

    if (neuron_groups == 0) {
        tw_error(TW_LOC, "Invalid number of neuron groups. There has to be at least one group.");
    }

    unsigned long const last_node = tw_nnodes() - 1;
    int32_t to_check_total_neurons = 0;
    for (uint8_t i = 0; i < neuron_groups; i++) {
        int32_t const num_neurons = load_int32(fp);
        layout_master_neurons(num_neurons, 0, last_node);
        to_check_total_neurons += num_neurons;
    }
    assert(total_num_neurons == to_check_total_neurons);

    for (uint8_t i = 0; i < synapse_groups; i++) {
        int32_t const from_start = load_int32(fp);
        int32_t const from_end   = load_int32(fp);
        int32_t const to_start   = load_int32(fp);
        int32_t const to_end     = load_int32(fp);
        layout_master_synapses_fully(from_start, from_end, to_start, to_end);
    }

    // Setting the driver configuration
    *settings_neuron_lp = (struct SettingsNeuronLP) {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = spikes,
      .beat              = beat,
      .firing_delay      = 1,
      .neuron_leak       = (neuron_leak_f) leak_lif_neuron,
      .neuron_leak_bigdt = (neuron_leak_big_f) leak_lif_big_neuron,
      .neuron_integrate  = (neuron_integrate_f) integrate_lif_neuron,
      .neuron_fire       = (neuron_fire_f) fire_lif_neuron,
      .store_neuron         = (neuron_state_op_f) store_lif_neuron_state,
      .reverse_store_neuron = (neuron_state_op_f) reverse_store_lif_neuron_state,
      //.print_neuron_struct  = (print_neuron_f) print_lif_neuron,
      //.gid_to_doryta_id    = ...
      //.probe_events     = probe_events,
    };

    // Allocates space for neurons and synapses
    layout_master_init(sizeof(struct LifNeuron),
            (neuron_init_f) NULL, (synapse_init_f) NULL);
    layout_master_configure(settings_neuron_lp);

    // Loading neuron and synapses from file
    // Assumes that increasing the local id also increases the doryta id,
    // which is true for layout/master
    int32_t i_in_file = 0;
    for (size_t i = 0; i < settings_neuron_lp->num_neurons_pe; i++) {
        // READ SYNAPSE params and its synapses, storing them directly on
        // CONTINUE FROM HERE!
        int32_t const doryta_id = local_id_to_doryta_id(i);
        //printf("PE %lu - neuron id %d - total_num_neurons %d\n", g_tw_mynode, doryta_id, num_synapses);
        assert((int32_t) doryta_id < total_num_neurons);

        // Seeking up to where doryta_id is stored
        // TODO: This loop is slow and wasteful with more than one PE. A single
        // `fseek` can be used instead. The problem lies on the number of
        // synapses that vary from one group to another
        while (i_in_file < doryta_id) {
            // Six parameters to ignore (all floats): potential, current,
            // resting_potential, reset_potential, threshold, tau, resistance
            fseek(fp, 7 * sizeof(float), SEEK_CUR);
            int32_t const num_synapses = load_int32(fp);
            if (num_synapses) {
                // We jump ahead one `int32` (to_start) and num_synapses
                // `float`
                fseek(fp, num_synapses * sizeof(float), SEEK_CUR);
            }
            i_in_file++;
        }

        // Storing neuron results
        struct LifNeuron * neuron = settings_neuron_lp->neurons[i];
        *neuron = (struct LifNeuron) {
            .potential         = load_float(fp),
            .current           = load_float(fp),
            .resting_potential = load_float(fp),
            .reset_potential   = load_float(fp),
            .threshold         = load_float(fp),
            .tau_m             = load_float(fp),
            .resistance        = load_float(fp),
        };

        int32_t const num_synapses = load_int32(fp);
        //printf("PE %lu - neuron id %d - num synapses %d\n", g_tw_mynode, doryta_id, num_synapses);
        assert((int32_t) settings_neuron_lp->synapses[i].num == num_synapses);

        // Loading synapses
        if (num_synapses) {
            float synapses_raw[num_synapses];
            load_floats(fp, synapses_raw, num_synapses);
            struct Synapse * synapses_neuron = settings_neuron_lp->synapses[i].synapses;
            for (int32_t j = 0; j < num_synapses; j++) {
                synapses_neuron[j].weight = synapses_raw[j];
            }
        }
        i_in_file++;
        /*
         *tw_error(TW_LOC, "first neuron doryta id %d - potential %f - current %f - "
         *        "resting_potential %f - reset %f - threshold %f - tau %f - n-synap %d",
         *        doryta_id, potential, current, resting_potential,
         *        reset_potential, threshold, tau, num_synapses);
         */
    }

    return (struct ModelParams) {
        .lps_in_pe = layout_master_total_lps_pe(),
        .gid_to_pe = layout_master_gid_to_pe,
    };
}


void model_load_neurons_deinit(void) {
    layout_master_free();
}

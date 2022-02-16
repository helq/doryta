#include "load_neurons.h"
#include "../../layout/master.h"
#include "../../layout/standard_layouts.h"
#include "../../neurons/lif.h"
#include "../../utils/io.h"


static void load_v1(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp);
static void load_v2(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp);

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
    if (format == 0x1) {
        load_v1(settings_neuron_lp, fp);
    } else if (format == 0x2) {
        load_v2(settings_neuron_lp, fp);
    } else {
        fclose(fp);
        tw_error(TW_LOC, "Input file corrupt or format unknown");
    }
    fclose(fp);

    return (struct ModelParams) {
        .lps_in_pe = layout_master_total_lps_pe(),
        .gid_to_pe = layout_master_gid_to_pe,
    };
}


static void load_neuron_params(struct LifNeuron * neuron, FILE * fp) {
    *neuron = (struct LifNeuron) {
        .potential         = load_float(fp),
        .current           = load_float(fp),
        .resting_potential = load_float(fp),
        .reset_potential   = load_float(fp),
        .threshold         = load_float(fp),
        .tau_m             = load_float(fp),
        .resistance        = load_float(fp),
    };
}


static void load_v1(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp) {
#ifndef NDEBUG
    int32_t const total_num_neurons =
#endif
    load_int32(fp);
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
      .spikes            = NULL,
      .beat              = beat,
      .neuron_leak       = (neuron_leak_f) neurons_lif_leak,
      .neuron_leak_bigdt = (neuron_leak_big_f) neurons_lif_big_leak,
      .neuron_integrate  = (neuron_integrate_f) neurons_lif_integrate,
      .neuron_fire       = (neuron_fire_f) neurons_lif_fire,
      .store_neuron         = (neuron_state_op_f) neurons_lif_store_state,
      .reverse_store_neuron = (neuron_state_op_f) neurons_lif_reverse_store_state,
      .print_neuron_struct  = (print_neuron_f) neurons_lif_print,
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
    for (int32_t i = 0; i < settings_neuron_lp->num_neurons_pe; i++) {
        // READ SYNAPSE params and its synapses, storing them directly on
        int32_t const doryta_id = layout_master_local_id_to_doryta_id(i);
        //printf("PE %lu - neuron id %d - total_num_neurons %d\n", g_tw_mynode, doryta_id, num_synapses);
        assert(doryta_id < total_num_neurons);

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
                fseek(fp, num_synapses * sizeof(float), SEEK_CUR);
            }
            i_in_file++;
        }

        // Storing neuron results
        load_neuron_params(settings_neuron_lp->neurons[i], fp);

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
                synapses_neuron[j].delay = 1;
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
}


struct Conv2dGroup {
    int32_t from_start;
    int32_t from_end;
    int32_t to_start;
    int32_t to_end;
    int32_t kernel_size;
    float * kernel_data;
};


static void load_v2(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp) {
#ifndef NDEBUG
    int32_t const total_num_neurons =
#endif
    load_int32(fp);
    uint16_t const neuron_groups = load_uint16(fp);
    uint16_t const synapse_groups = load_uint16(fp);
    float const beat = load_float(fp);

    if (neuron_groups == 0) {
        tw_error(TW_LOC, "Invalid number of neuron groups. There has to be at least one group.");
    }

    unsigned long const last_node = tw_nnodes() - 1;
    int32_t to_check_total_neurons = 0;
    for (uint16_t i = 0; i < neuron_groups; i++) {
        int32_t const num_neurons = load_int32(fp);
        layout_master_neurons(num_neurons, 0, last_node);
        to_check_total_neurons += num_neurons;
    }
    assert(total_num_neurons == to_check_total_neurons);

    // Loading layout/connections
    struct Conv2dGroup conv_kernels[synapse_groups]; // A bit wasteful, but simple to implement
    uint16_t n_convs = 0;

    for (uint16_t i = 0; i < synapse_groups; i++) {
        uint8_t const conn_type = load_uint8(fp);

        int32_t const from_start = load_int32(fp);
        int32_t const from_end   = load_int32(fp);
        int32_t const to_start   = load_int32(fp);
        int32_t const to_end     = load_int32(fp);

        if (conn_type == 0x1) {
            layout_master_synapses_fully(from_start, from_end, to_start, to_end);
        } else if (conn_type == 0x2) {
            /*int32_t const input_height =*/ load_int32(fp);
            int32_t const input_width     = load_int32(fp);
            /*int32_t const output_height =*/ load_int32(fp);
            /*int32_t const output_width =*/ load_int32(fp);
            int32_t const padding_height  = load_int32(fp);
            int32_t const padding_width   = load_int32(fp);
            int32_t const striding_height = load_int32(fp);
            int32_t const striding_width  = load_int32(fp);
            int32_t const kernel_height   = load_int32(fp);
            int32_t const kernel_width    = load_int32(fp);
            struct Conv2dParams const conv2d_params = {
                    .input_width    = input_width,
                    .kernel_width   = kernel_width,
                    .kernel_height  = kernel_height,
                    .padding_width  = padding_width,
                    .padding_height = padding_height,
                    .stride_width   = striding_width,
                    .stride_height  = striding_height,
            };

            layout_master_synapses_conv2d(
                    from_start, from_end, to_start, to_end,
                    &conv2d_params);

            int32_t kernel_size = kernel_height * kernel_width;
            float * kernel_data = malloc(kernel_size * sizeof(float));
            load_floats(fp, kernel_data, kernel_size);
            conv_kernels[n_convs] = (struct Conv2dGroup) {
                .from_start = from_start,
                .from_end = from_end,
                .to_start = to_start,
                .to_end = to_end,
                .kernel_size = kernel_size,
                .kernel_data = kernel_data
            };
            n_convs++;
        } else {
            tw_error(TW_LOC, "Unknown layout type `%x`.", conn_type);
        }
    }

    // Setting the driver configuration
    *settings_neuron_lp = (struct SettingsNeuronLP) {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = NULL,
      .beat              = beat,
      .neuron_leak       = (neuron_leak_f) neurons_lif_leak,
      .neuron_leak_bigdt = (neuron_leak_big_f) neurons_lif_big_leak,
      .neuron_integrate  = (neuron_integrate_f) neurons_lif_integrate,
      .neuron_fire       = (neuron_fire_f) neurons_lif_fire,
      .store_neuron         = (neuron_state_op_f) neurons_lif_store_state,
      .reverse_store_neuron = (neuron_state_op_f) neurons_lif_reverse_store_state,
      .print_neuron_struct  = (print_neuron_f) neurons_lif_print,
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
    for (int32_t i = 0; i < settings_neuron_lp->num_neurons_pe; i++) {
        // READ SYNAPSE params and its synapses, storing them directly on structure
        int32_t const doryta_id = layout_master_local_id_to_doryta_id(i);
        //printf("PE %lu - neuron id %d - total_num_neurons %d\n", g_tw_mynode, doryta_id, num_synapses);
        assert(doryta_id < total_num_neurons);

        // Seeking up to where doryta_id is stored
        while (i_in_file < doryta_id) {
            // Six parameters to ignore (all floats): potential, current,
            // resting_potential, reset_potential, threshold, tau, resistance
            fseek(fp, 7 * sizeof(float), SEEK_CUR);
            uint16_t const num_groups_fully = load_uint16(fp);
            for (uint16_t j = 0; j < num_groups_fully; j++) {
                int32_t const to_start = load_int32(fp);
                int32_t const to_end = load_int32(fp);
                int32_t const num_synapses = to_end - to_start + 1;

                // We jump ahead num_synapses `float`
                fseek(fp, num_synapses * sizeof(float), SEEK_CUR);
            }
            i_in_file++;
        }

        // Storing neuron results
        load_neuron_params(settings_neuron_lp->neurons[i], fp);

        uint16_t const num_groups_fully = load_uint16(fp);
        uint16_t group_ind = 0;
        uint16_t conv_ind = 0;

        int32_t to_start_fully;
        int32_t to_end_fully;
        if (group_ind < num_groups_fully) {
            to_start_fully = load_int32(fp);
            to_end_fully = load_int32(fp);
        }

        int32_t const num_synapses = settings_neuron_lp->synapses[i].num;
        struct Synapse * synapses_neuron = settings_neuron_lp->synapses[i].synapses;
        for (int32_t j = 0; j < num_synapses; j++) {
            bool neither = true; // wasn't the neuron loaded by neither fully or conv parameters?
            int32_t const to_id = synapses_neuron[j].doryta_id_to_send;

            // check if id corresponds to to_start in current fully layer
            if (group_ind < num_groups_fully && to_id == to_start_fully) {
                // load the entirety of synapses
                int32_t const num_synapses_group = to_end_fully - to_start_fully + 1;

                float synapses_raw[num_synapses_group];
                load_floats(fp, synapses_raw, num_synapses_group);
                for (int32_t k = 0; k < num_synapses_group; k++) {
                    assert(synapses_neuron[j + k].doryta_id_to_send == to_start_fully + k);
                    synapses_neuron[j + k].weight = synapses_raw[k];
                    synapses_neuron[j + k].delay = 1;
                }

                // we loaded a bunch of neurons at the same time!
                j += num_synapses_group - 1;

                // getting ready for next group
                group_ind++;
                if (group_ind < num_groups_fully) {
                    to_start_fully = load_int32(fp);
                    to_end_fully = load_int32(fp);
                }

                neither = false;

            // check if there are still convolutions to check for to_id to belong to
            } else if (conv_ind < n_convs) {
                // find next convolution group in which the current
                // neuron (doryta_id) appears and to_id belongs in the range (to_start, to_end)
                while (conv_ind < n_convs) {
                    struct Conv2dGroup * group = &conv_kernels[conv_ind];
                    if (group->from_start <= doryta_id
                            && doryta_id <= group->from_end
                            && group->to_start <= to_id
                            && to_id <= group->to_end) {
                        break;
                    }
                    conv_ind++;
                }

                // if a group were found, load from kernel value that synapse should have
                if (conv_ind < n_convs) {
                    // get the value stored in weight (it indicates which value of the kernel convolution to use)
                    int32_t const kernel_id = (int32_t) synapses_neuron[j].weight;
                    assert(kernel_id < conv_kernels[conv_ind].kernel_size);
                    synapses_neuron[j].weight = conv_kernels[conv_ind].kernel_data[kernel_id];
                    synapses_neuron[j].delay = 1;

                    // this advances neurons one at the time, no need to alter j
                    neither = false;
                }
            }

            // if neither occurs, throw exception
            if (neither) {
                tw_error(TW_LOC, "The synapse connecting neurons %d to %d could not be stored.",
                        doryta_id, to_id);
            }
        }

        i_in_file++;
    }

    // Freeing dynamic memory used to load kernel params
    for (uint16_t i = 0; i < n_convs; i++) {
        free(conv_kernels[i].kernel_data);
    }
}


void model_load_neurons_deinit(void) {
    layout_master_free();
}

#include "load_neurons.h"
#include "../strategy/modes.h"
#include "../strategy/layouts/master.h"
#include "../strategy/layouts/standard_layouts.h"
#include "../../neurons/lif.h"
#include "../../utils/io.h"

static bool is_initial_current_nonzero = false;
static bool is_reset_higher_than_treshold = false;

static bool is_using_layout = false;

// In case we are not using the layout strategy, but just raw data/arbitrary connections
static int32_t                  total_neurons = -1;
static bool                     initialized_arbitrary = false;
static struct Synapse           * naked_synapses = NULL;
static char                     * naked_neurons = NULL;
// To pass to SettingsNeuronLP
static void                    ** neurons = NULL;
static struct SynapseCollection * synapses = NULL;

static void load_with_layout_v1(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp);
static void load_with_layout_v2(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp);
static void load_arbitrary_single_pe(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp);

static size_t get_total_neurons_arbitrary(void);
static int32_t gid_to_doryta_id_identity(size_t gid);
static unsigned long gid_to_pe_zero(uint64_t gid);
static unsigned long doryta_id_to_pe_zero(int32_t doryta_id);
static size_t doryta_id_to_local_id_identity(int32_t doryta_id);


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
        load_with_layout_v1(settings_neuron_lp, fp);
        is_using_layout = true;
    } else if (format == 0x2) {
        load_with_layout_v2(settings_neuron_lp, fp);
        is_using_layout = true;
    } else if (format == 0x11) {
        load_arbitrary_single_pe(settings_neuron_lp, fp);
    } else {
        fclose(fp);
        tw_error(TW_LOC, "Input file corrupt or format unknown");
    }
    fclose(fp);

    if (is_initial_current_nonzero) {
        tw_warning(TW_LOC, "Some neurons start the simulation with current != 0, which "
                "does not properly work with spike-driven mode");
    }
    if (is_reset_higher_than_treshold) {
        tw_warning(TW_LOC, "Some neurons' threshold are lower than their reset potential. "
                "This leads a neuron to spike on every time step and will not properly "
                "behave on spike-driven mode.");
    }

    assert((total_neurons == -1) == is_using_layout);
    const struct StrategyParams mode_params = get_model_strategy_mode();
    assert(mode_params.mode != MODEL_LOADING_MODE_none_yet);
    assert(is_using_layout == (mode_params.mode == MODEL_LOADING_MODE_layouts));

    return (struct ModelParams) {
        .lps_in_pe = mode_params.lps_in_pe(),
        .gid_to_pe = mode_params.gid_to_pe,
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

    // checking if conditions for Spike-driven mode are fulfilled
    if (!is_reset_higher_than_treshold && neuron->threshold < neuron->reset_potential) {
        is_reset_higher_than_treshold = true;
    }
    if (!is_initial_current_nonzero && neuron->current != 0) {
        is_initial_current_nonzero = true;
    }
}


static void load_with_layout_v1(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp) {
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
        layout_master_synapses_all2all(from_start, from_end, to_start, to_end);
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


static void load_with_layout_v2(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp) {
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
            layout_master_synapses_all2all(from_start, from_end, to_start, to_end);
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

        // Arbitrary initial value of 1 and 0 to keep compiler happy. The actual values are
        // read from file
        int32_t to_start_fully = 1;
        int32_t to_end_fully = 0;
        if (group_ind < num_groups_fully) {
            to_start_fully = load_int32(fp);
            to_end_fully = load_int32(fp);
        }

        int32_t const num_synapses = settings_neuron_lp->synapses[i].num;
        struct Synapse * synapses_neuron = settings_neuron_lp->synapses[i].synapses;
        for (int32_t j = 0; j < num_synapses; j++) {
            bool neither = true; // wasn't the neuron loaded by neither fully or conv parameters?
#ifdef NDEBUG
            int32_t const to_id = layout_master_gid_to_doryta_id(synapses_neuron[j].gid_to_send);
#else
            assert(layout_master_gid_to_doryta_id(synapses_neuron[j].gid_to_send)
                    == synapses_neuron[j].doryta_id_to_send);
            int32_t const to_id = synapses_neuron[j].doryta_id_to_send;
#endif

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


static void load_arbitrary_single_pe(struct SettingsNeuronLP * settings_neuron_lp, FILE * fp) {
    uint8_t neuron_type = load_uint8(fp);
    if (neuron_type != 0x1) {
        tw_error(TW_LOC, "Unsupported neuron type (%x)", neuron_type);
    }

    total_neurons = load_int32(fp);
    int32_t const total_neurons_in_pe = total_neurons;
    int64_t const total_synapses = load_int64(fp);
    double const beat = load_double(fp);

    if (tw_nnodes() != 1) {
        tw_error(TW_LOC, "This file type can only load neurons for a single PE/core/MPI rank");
    }

    // Allocating space
    int sizeof_neuron = sizeof(struct LifNeuron);
    synapses =
        malloc(total_neurons_in_pe * sizeof(struct SynapseCollection));
    naked_synapses =
        malloc(total_synapses * sizeof(struct Synapse));
    neurons =
        malloc(total_neurons_in_pe * sizeof(void*));
    naked_neurons =
        malloc(total_neurons_in_pe * sizeof_neuron);
    initialized_arbitrary = true;

    if (synapses == NULL
       || naked_synapses == NULL
       || naked_neurons == NULL
       || neurons == NULL) {
        tw_error(TW_LOC, "Not able to allocate space for neurons and synapses");
    }

    // Loading neurons params and synapses from file
    int64_t synapses_i = 0;
    for (int32_t i = 0; i < total_neurons_in_pe; i++) {
        assert(i < total_neurons);
        assert(synapses_i < total_synapses);

        // Connecting neurons pointers to naked neuron array
        neurons[i] = (void*) & naked_neurons[i*sizeof_neuron];
        // Loading neuron params
        load_neuron_params(neurons[i], fp);

        int32_t const num_synapses = load_int32(fp);
        synapses[i].num = num_synapses;
        synapses[i].synapses = NULL;

        // Loading synapses
        if (num_synapses) {
            int32_t synapses_ids[num_synapses];
            float synapses_weights[num_synapses];
            int32_t synapses_delays[num_synapses];
            load_int32s(fp, synapses_ids, num_synapses);
            load_floats(fp, synapses_weights, num_synapses);
            load_int32s(fp, synapses_delays, num_synapses);
            struct Synapse * synapses_neuron = &naked_synapses[synapses_i];
            synapses[i].num = num_synapses;
            synapses[i].synapses = synapses_neuron;
            for (int32_t j = 0; j < num_synapses; j++) {
                synapses_neuron[j].gid_to_send = synapses_ids[j];
#ifndef NDEBUG
                synapses_neuron[j].doryta_id_to_send = synapses_ids[j];
#endif
                synapses_neuron[j].weight = synapses_weights[j];
                synapses_neuron[j].delay = synapses_delays[j];
            }
        }

        synapses_i += num_synapses;
    }
    assert(total_synapses == synapses_i);

    // Setting the driver configuration
    *settings_neuron_lp = (struct SettingsNeuronLP) {
      .num_neurons      = total_neurons,
      .num_neurons_pe   = total_neurons_in_pe,
      .neurons          = neurons,
      .synapses         = synapses,
      .spikes            = NULL,
      .beat              = beat,
      .neuron_leak       = (neuron_leak_f) neurons_lif_leak,
      .neuron_leak_bigdt = (neuron_leak_big_f) neurons_lif_big_leak,
      .neuron_integrate  = (neuron_integrate_f) neurons_lif_integrate,
      .neuron_fire       = (neuron_fire_f) neurons_lif_fire,
      .store_neuron         = (neuron_state_op_f) neurons_lif_store_state,
      .reverse_store_neuron = (neuron_state_op_f) neurons_lif_reverse_store_state,
      .print_neuron_struct  = (print_neuron_f) neurons_lif_print,
      .gid_to_doryta_id    = gid_to_doryta_id_identity,
      //.probe_events     = probe_events,
    };

    set_model_strategy_mode(&(const struct StrategyParams) {
            .mode = MODEL_LOADING_MODE_custom,
            .lps_in_pe = get_total_neurons_arbitrary,
            .gid_to_pe = gid_to_pe_zero,
            .doryta_id_to_pe = doryta_id_to_pe_zero,
            .doryta_id_to_local_id = doryta_id_to_local_id_identity
    });
}


void model_load_neurons_deinit(void) {
    if (is_using_layout) {
        layout_master_free();
    } else {
        assert(initialized_arbitrary);
        free(naked_synapses);
        free(naked_neurons);
        free(neurons);
        free(synapses);
        total_neurons = -1;
        initialized_arbitrary = false;
    }
}

/* Supporting functions for ids convertions -- Only one PE case */

static int32_t gid_to_doryta_id_identity(size_t gid) {
    return gid;
}

static unsigned long gid_to_pe_zero(uint64_t gid) {
    (void) gid;
    return 0;
}

static size_t get_total_neurons_arbitrary(void) {
    assert(!is_using_layout);
    return total_neurons;
}

static unsigned long doryta_id_to_pe_zero(int32_t doryta_id) {
    (void) doryta_id;
    return 0;
}

static size_t doryta_id_to_local_id_identity(int32_t doryta_id) {
    return doryta_id;
}

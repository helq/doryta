#include "gameoflife.h"
#include "../../layout/master.h"
#include "../../layout/standard_layouts.h"
#include "../../neurons/lif.h"

// Defined in ross.h
unsigned int tw_nnodes(void);

static int32_t width_world = 20;  // Default size of GoL is 20x20


// Life cell:
//   should be activated with 3 spikes
// Kill cell:
//   should be activated with 4 spikes
// Board cell:
//   It's activated with 1 spike (kill cell spikes negatively, so
//   1 spike from a Kill cell will be 0 spikes in total)
static void initialize_LIF(struct LifNeuron * lif, int32_t doryta_id) {
    int32_t const size_world = width_world * width_world;

    assert(doryta_id < 3 * size_world);
    float threshold = 0;
    if (doryta_id < size_world) {
        threshold = 0.9;
    } else if (doryta_id < 2 * size_world) {
        threshold = 2.9;
    } else /* if (doryta_id < 3 * size_world) */ {
        threshold = 3.9;
    }
    *lif = (struct LifNeuron) {
        .potential = 0,
        .current = 0,
        .resting_potential = 0,
        .reset_potential = 0,
        .threshold = threshold,
        .tau_m = .5,
        .resistance = 1
    };
}

// The kernels to implement:
// Board -> Life
//   [1, 1, 1,
//    1, 1, 1,
//    1, 1, 1]
// Board -> Kill
//   [1, 1, 1,
//    1, 0, 1,
//    1, 1, 1]
// Life -> Board
//   [[1], [-1]]
// Kill -> Board
//   [-1]
static float initialize_weight_neurons(int32_t neuron_from, int32_t neuron_to) {
    int32_t const size_world = width_world * width_world;

    float weight = 0.0;
    if (neuron_from < size_world) {  // From Board cell
        assert(neuron_to >= size_world);
        if (neuron_to < 2 * size_world) {  // To Life cell
            weight = 1;
        } else if (neuron_to < 3 * size_world) { // To Kill cell
            weight = neuron_to - 2 * size_world == neuron_from ? 0 : 1;
        }
    } else if (neuron_from < 2 * size_world) { // From Life cell
        assert(neuron_to < size_world);
        weight = 1;
    } else if (neuron_from < 3 * size_world) { // From Kill cell
        assert(neuron_to < size_world);
        weight = -1;
    }
    return weight;
}


struct ModelParams
model_GoL_neurons_init(
        struct SettingsNeuronLP * settings_neuron_lp,
        unsigned int width_world_) {
    // Careful, a mistmatch in `unsigned int` and `int32_t` might
    // bring some trouble, but it probably won't
    width_world = width_world_;

    // Setting the driver configuration
    *settings_neuron_lp = (struct SettingsNeuronLP) {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = NULL,
      .beat              = .5,
      .firing_delay      = 1,
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

    unsigned long const last_node = tw_nnodes()-1;
    int32_t const size_world = width_world * width_world;
    int32_t const layer_1st_start = 0;
    int32_t const layer_2nd_start = size_world;
    int32_t const layer_3rd_start = 2 * size_world;
    int32_t const layer_1st_end = size_world - 1;
    int32_t const layer_2nd_end = 2 * size_world - 1;
    int32_t const layer_3rd_end = 3 * size_world - 1;
    // Defining neurons
    // GoL board
    layout_master_neurons(size_world, 0, last_node);
    // Fire if life (either birth or sustain life)
    layout_master_neurons(size_world, 0, last_node);
    // Fire if kill (more than 4 neighbors, either kill or prevent birth)
    layout_master_neurons(size_world, 0, last_node);

    // Defining synapses
    struct Conv2dParams const conv2d_params = {
            .input_width    = width_world,
            .kernel_width   = 3,
            .kernel_height  = 3,
            .padding_width  = 1,
            .padding_height = 1,
            .stride_width  = 1,
            .stride_height = 1,
    };
    layout_master_synapses_conv2d(
            layer_1st_start, layer_1st_end,
            layer_2nd_start, layer_2nd_end,
            &conv2d_params);
    layout_master_synapses_conv2d(
            layer_1st_start, layer_1st_end,
            layer_3rd_start, layer_3rd_end,
            &conv2d_params);
    struct Conv2dParams const one2one_params = {
            .input_width    = width_world,
            .kernel_width   = 1,
            .kernel_height  = 1,
            .padding_width  = 0,
            .padding_height = 0,
            .stride_width  = 1,
            .stride_height = 1,
    };
    layout_master_synapses_conv2d(
            layer_2nd_start, layer_2nd_end,
            layer_1st_start, layer_1st_end,
            &one2one_params);
    layout_master_synapses_conv2d(
            layer_3rd_start, layer_3rd_end,
            layer_1st_start, layer_1st_end,
            &one2one_params);

    // Allocates space for neurons and synapses
    layout_master_init(sizeof(struct LifNeuron),
            (neuron_init_f) initialize_LIF,
            (synapse_init_f) initialize_weight_neurons);
    layout_master_configure(settings_neuron_lp);

    return (struct ModelParams) {
        .lps_in_pe = layout_master_total_lps_pe(),
        .gid_to_pe = layout_master_gid_to_pe,
    };
}


void model_GoL_neurons_deinit(void) {
    layout_master_free();
}

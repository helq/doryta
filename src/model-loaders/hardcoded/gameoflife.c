#include "gameoflife.h"
#include "../../layout/master.h"
#include "../../layout/standard_layouts.h"
#include "../../neurons/lif.h"

// Defined in ross.h
unsigned int tw_nnodes(void);


// Life cell:
//   should be activated with 3 spikes
// Kill cell:
//   should be activated with 4 spikes
// Board cell:
//   It's activated with 1 spike (kill cell spikes negatively, so
//   1 spike from a Kill cell will be 0 spikes in total)
static void initialize_LIF(struct LifNeuron * lif, int32_t doryta_id) {
    assert(doryta_id < 1200);
    float threshold = 0;
    if (doryta_id < 400) {
        threshold = 0.9;
    } else if (doryta_id < 800) {
        threshold = 2.9;
    } else if (doryta_id < 1200) {
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
    float weight = 0.0;
    if (neuron_from < 400) {  // From Board cell
        assert(neuron_to >= 400);
        if (neuron_to < 800) {  // To Life cell
            weight = 1;
        } else if (neuron_to < 1200) { // To Kill cell
            weight = neuron_to - 800 == neuron_from ? 0 : 1;
        }
    } else if (neuron_from < 800) { // From Life cell
        assert(neuron_to < 400);
        weight = 1;
    } else if (neuron_from < 1200) { // From Kill cell
        assert(neuron_to < 400);
        weight = -1;
    }
    return weight;
}


struct ModelParams
model_GoL_neurons_init(struct SettingsNeuronLP * settings_neuron_lp) {
    // Setting the driver configuration
    *settings_neuron_lp = (struct SettingsNeuronLP) {
      //.num_neurons      = ...
      //.num_neurons_pe   = ...
      //.neurons          = ...
      //.synapses         = ...
      .spikes            = NULL,
      .beat              = .5,
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

    unsigned long const last_node = tw_nnodes()-1;
    // GoL board
    layout_master_neurons(400, 0, last_node);
    // Fire if life (either birth or sustain life)
    layout_master_neurons(400, 0, last_node);
    // Fire if kill (more than 4 neighbors, either kill or prevent birth)
    layout_master_neurons(400, 0, last_node);
    struct Conv2dParams const conv2d_params = {
            .input_width    = 20,
            .kernel_width   = 3,
            .kernel_height  = 3,
            .padding_width  = 1,
            .padding_height = 1,
    };
    layout_master_synapses_conv2d(0, 399, 400, 799,  &conv2d_params);
    layout_master_synapses_conv2d(0, 399, 800, 1199, &conv2d_params);
    struct Conv2dParams const one2one_params = {
            .input_width    = 20,
            .kernel_width   = 1,
            .kernel_height  = 1,
            .padding_width  = 0,
            .padding_height = 0,
    };
    layout_master_synapses_conv2d(400, 799,  0, 399, &one2one_params);
    layout_master_synapses_conv2d(800, 1199, 0, 399, &one2one_params);

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

#include "master.h"
#include <assert.h>
#include <stdbool.h>
#include <ross.h>

struct LayoutLevelParams {
    enum LAYOUT_TYPE layoutType;
    size_t total_neurons;
    size_t initial_pe;
    size_t final_pe;
    size_t neurons_in_pe;
    size_t local_id_offset;
    size_t doryta_id_offset;
    size_t n_synapses;
    size_t synapses_offset;
};

#define MAX_LAYOUTS 20

static size_t                   num_layouts = 0;
static struct LayoutLevelParams layout_levels[MAX_LAYOUTS];
static size_t                   max_num_neurons_per_pe = 0;
static size_t                   total_neurons_in_pe = 0;
static size_t                   total_synapses = 0;

static struct MemoryAllocationLayout allocation_layout = {0};


size_t layout_master_reserve_neurons(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe,
        enum LAYOUT_TYPE layoutType) {
    if (initial_pe >= tw_nnodes() || final_pe >= tw_nnodes()) {
        tw_error(TW_LOC, "Valid PE ids for initial and final PEs must be smaller"
                         " than the number of nodes in the system");
    }
    if(num_layouts >= MAX_LAYOUTS) {
        tw_error(TW_LOC, "Only up to %d layout levels are allowed", MAX_LAYOUTS);
    }

    // The initial and final PEs can be 0 and tw_nnodes(), or they could be,
    // 0 and 1, or even they could be `tw_nnodes() - 20` and `10`. In this
    // later case, the layout spands a total of 31 PEs, starting from a
    // position at the tail of the PE ID numbering and continuing from the
    // start.
    const unsigned long total_pes = initial_pe <= final_pe ?
        final_pe - initial_pe + 1
        : tw_nnodes() - (initial_pe - final_pe - 1);
    const bool pe_inside_range = initial_pe <= final_pe ?
        initial_pe <= g_tw_mynode && g_tw_mynode <= final_pe
        : initial_pe <= g_tw_mynode || g_tw_mynode <= final_pe;
    const unsigned long index_within_range = initial_pe <= g_tw_mynode ?
        g_tw_mynode - initial_pe
        : g_tw_mynode + (tw_nnodes() - initial_pe);
    // The reminder neurons appear when the total number of neurons cannot be
    // divided exactly in the number of PEs. To balance the number of neurons
    // per PE we give an additional neuron to the first `total_neurons %
    // total_pes` PEs
    const size_t reminder_neurons = total_neurons % total_pes;
    const size_t neurons_in_pe =
        pe_inside_range ?
        total_neurons / total_pes + (index_within_range < reminder_neurons)
        : 0;
    const unsigned long doryta_id_offset =
        total_neurons_in_pe
        + index_within_range * (total_neurons / total_pes)
        + (index_within_range < reminder_neurons ?
                index_within_range : reminder_neurons);

    layout_levels[num_layouts] = (struct LayoutLevelParams){
        .layoutType = layoutType,
        .total_neurons = total_neurons,
        .initial_pe = initial_pe,
        .final_pe = final_pe,
        .neurons_in_pe = neurons_in_pe,
        .local_id_offset = total_neurons_in_pe,
        .doryta_id_offset = doryta_id_offset,
        .n_synapses = 0,
        .synapses_offset = 0,
    };
    //printf("PE %lu - total neurons %lu - neurons in pe %lu - offset %lu\n",
    //        g_tw_mynode, total_neurons, neurons_in_pe, doryta_id_offset);

    max_num_neurons_per_pe += ceil((double) total_neurons / total_pes);
    total_neurons_in_pe += neurons_in_pe;

    const size_t current_layout_level = num_layouts;
    num_layouts++;
    return current_layout_level;
}


size_t layout_master_reserve_synapses(size_t num_layout_level, size_t n_synapses) {
    assert(num_layout_level < num_layouts);
    assert(layout_levels[num_layout_level].n_synapses == 0);

    const size_t synapses_offset = total_synapses;
    layout_levels[num_layout_level].n_synapses = n_synapses;
    layout_levels[num_layout_level].synapses_offset = synapses_offset;
    total_synapses += n_synapses;
    return synapses_offset;
}


void layout_master_init(int sizeof_neuron) {
    (void) sizeof_neuron;

    allocation_layout.synapses =
        malloc(total_neurons_in_pe * sizeof(struct SynapseCollection));
    allocation_layout.naked_synapses =
        malloc(total_synapses * sizeof(struct Synapse));
    allocation_layout.neurons =
        malloc(total_neurons_in_pe * sizeof(void*));
    allocation_layout.naked_neurons =
        malloc(total_neurons_in_pe * sizeof_neuron);

    if (allocation_layout.synapses == NULL
       || allocation_layout.naked_synapses == NULL
       || allocation_layout.naked_neurons == NULL
       || allocation_layout.neurons == NULL) {
        tw_error(TW_LOC, "Not able to allocate space for neurons and synapses");
    }

    // Connecting neurons pointers to naked neuron array
    for (size_t i = 0; i < total_neurons_in_pe; i++) {
        allocation_layout.neurons[i] = (void*) & allocation_layout.naked_neurons[i];
    }

    //Custom Mapping
    /*
    g_tw_mapping = CUSTOM;
    g_tw_custom_initial_mapping = &map_pseudo_linear;
    g_tw_custom_lp_global_to_local_map = &gid_to_local_lp_id;
    */

    // IF there are multiple LP types
    //    you should define the mapping of GID -> lptype index
    //g_tw_lp_typemap = &model_typemap;
    //
}

void layout_master_free(void) {
    free(allocation_layout.naked_synapses);
    free(allocation_layout.neurons);
    free(allocation_layout.synapses);
}

struct MemoryAllocationLayout
layout_master_allocation(void) {
    assert(num_layouts > 0);
    return allocation_layout;
}

void layout_master_configure(struct SettingsNeuronLP *settingsNeuronLP) {
    settingsNeuronLP->neurons = allocation_layout.neurons;
    settingsNeuronLP->synapses = allocation_layout.synapses;
}

// Master defines mapping for everything in this PE. It's an alteration on linear mapping
//
//static void map_pseudo_linear(void) {
//
//    unsigned int nlp_per_kp;
//    tw_lpid  lpid;
//    tw_kpid  kpid;
//    unsigned int j;
//
//    // may end up wasting last KP, but guaranteed each KP has == nLPs
//    nlp_per_kp = (int)ceil((double) g_tw_nlp / (double) g_tw_nkp);
//
//    if(!nlp_per_kp) {
//        tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);
//    }
//
//    g_tw_lp_offset = g_tw_mynode * g_tw_nlp; // THIS IS DIFFERENT!!
//    //g_tw_lp_offset = g_tw_mynode * max_num_neurons_per_pe;
//
//#if VERIFY_MAPPING
//    printf("NODE %d: nlp %lld, offset %lld\n", g_tw_mynode, g_tw_nlp, g_tw_lp_offset);
//    printf("\tPE %d\n", g_tw_pe->id);
//#endif
//
//    for(kpid = 0, lpid = 0; kpid < nkp_per_pe; kpid++) {
//        tw_kp_onpe(kpid, g_tw_pe);
//
//#if VERIFY_MAPPING
//        printf("\t\tKP %d", kpid);
//#endif
//
//        for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++) {
//            tw_lp_onpe(lpid, g_tw_pe, g_tw_lp_offset+lpid);
//            tw_lp_onkp(g_tw_lp[lpid], g_tw_kp[kpid]);
//
//#if VERIFY_MAPPING
//            if(0 == j % 20) {
//                printf("\n\t\t\t");
//            }
//            printf("%lld ", lpid+g_tw_lp_offset);
//#endif
//        }
//
//#if VERIFY_MAPPING
//        printf("\n");
//#endif
//    }
//
//    if(!g_tw_lp[g_tw_nlp-1]) {
//        tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);
//    }
//
//    if(g_tw_lp[g_tw_nlp-1]->gid != g_tw_lp_offset + g_tw_nlp - 1) {
//        tw_error(TW_LOC, "LPs not sequentially enumerated!");
//    }
//}
//
//
////Given a gid, return the local LP (global id => local id mapping)
//tw_lp * gid_to_local_lp_id(tw_lpid){
//  int local_id = lp_id - g_tw_offset;
//  return g_tw_lp[id];
//}
//
// There are functions to define here!
// - GID to model type
// - GID to PE (for fast location, basically, the same shit as always, this is plugged directly in the LP definitions)
// - GID to DorytaID (because neurons should know their neurons IDs and to which neuron to route given their DorytaID)
// - DorytaID to GID
// - DorytaID to ID
// - ID to DorytaID

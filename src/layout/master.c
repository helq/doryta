#include "master.h"
#include <assert.h>
#include <stdbool.h>
#include <ross.h>

#define MAX_NEURON_GROUPS 20
#define MAX_SYNAPSE_GROUPS 100

// Parameters for each neuron group defined
struct NeuronGroup {
    // global parameters
    size_t num_neurons;
    size_t initial_pe;
    size_t final_pe;
    size_t total_pes;
    size_t global_neuron_offset;

    // local (PE) parameters
    size_t neurons_in_pe;
    size_t local_id_offset;
    size_t doryta_id_offset;
};

struct SynapseGroup {
    size_t from_start;
    size_t from_end;
    size_t to_start;
    size_t to_end;
};


// Parameters that define each neuron layer (only ids and number of neurons,
// not connections between them)
static int                num_neuron_groups = 0;
static struct NeuronGroup neuron_groups[MAX_NEURON_GROUPS];
static size_t             max_num_neurons_per_pe = 0;
static size_t             total_neurons_globally = 0;
static size_t             total_neurons_in_pe = 0;

// Parameters that define synapse connections
static int                 num_synap_groups = 0;
static struct SynapseGroup synapse_groups[MAX_SYNAPSE_GROUPS];
static size_t              total_synapses = 0; // total synapses in PE

// Allocation variables
static bool                     initialized = false;
static struct Synapse           * naked_synapses = NULL;
static char                     * naked_neurons = NULL;
// The two below is what we pass to SettingsNeuronLP
static void                    ** neurons = NULL; // In simulation time this ends up never been used, just in initialization
static struct SynapseCollection * synapses = NULL;

// To be used for "linear" mapping
static uint64_t pe_gid_offset;


size_t layout_master_neurons(
        size_t total_neurons, unsigned long initial_pe, unsigned long final_pe) {
    if (initial_pe >= tw_nnodes() || final_pe >= tw_nnodes()) {
        tw_error(TW_LOC, "Valid PE ids for initial and final PEs must be smaller"
                         " than the number of nodes in the system");
    }
    if(num_neuron_groups >= MAX_NEURON_GROUPS) {
        tw_error(TW_LOC, "Only up to %d neuron groups are allowed", MAX_NEURON_GROUPS);
    }

    // The initial and final PEs can be 0 and tw_nnodes(), or they could be,
    // 0 and 1, or even they could be `tw_nnodes() - 20` and `10`. In this
    // later case, the layout spands a total of 31 PEs, starting from a
    // position at the tail of the PE ID numbering and continuing from the
    // start.
    size_t const my_pe = g_tw_mynode;
    size_t const max_pes = tw_nnodes();
    size_t const total_pes = initial_pe <= final_pe ?
        final_pe - initial_pe + 1
        : max_pes - (initial_pe - final_pe - 1);
    bool const pe_inside_range = initial_pe <= final_pe ?
        initial_pe <= my_pe && my_pe <= final_pe
        : initial_pe <= my_pe || my_pe <= final_pe;
    size_t const index_within_range = initial_pe <= my_pe ?
        my_pe - initial_pe
        : my_pe + (max_pes - initial_pe);
    // The reminder neurons appear when the total number of neurons cannot be
    // divided exactly in the number of PEs. To balance the number of neurons
    // per PE we give an additional neuron to the first `total_neurons %
    // total_pes` PEs
    size_t const remainder_neurons = total_neurons % total_pes;
    size_t const neurons_in_pe =
        pe_inside_range ?
        total_neurons / total_pes + (index_within_range < remainder_neurons)
        : 0;
    size_t const doryta_id_offset =
        total_neurons_globally
        + index_within_range * (total_neurons / total_pes)
        + (index_within_range < remainder_neurons ?
                index_within_range : remainder_neurons);

    neuron_groups[num_neuron_groups] = (struct NeuronGroup){
        .num_neurons = total_neurons,
        .initial_pe = initial_pe,
        .final_pe = final_pe,
        .total_pes = total_pes,
        .global_neuron_offset = total_neurons_globally,

        .neurons_in_pe = neurons_in_pe,
        .local_id_offset = total_neurons_in_pe,
        .doryta_id_offset = doryta_id_offset,
        //.gid_offset = -1,
    };
    num_neuron_groups++;

    max_num_neurons_per_pe += ceil((double) total_neurons / total_pes);
    total_neurons_in_pe += neurons_in_pe;
    size_t const to_ret = total_neurons_globally;
    total_neurons_globally += total_neurons;

    /*
     *printf("PE %lu - total neurons %lu - neurons in pe %lu - offset %lu "
     *        "- total_neurons_globally = %lu\n",
     *        g_tw_mynode, total_neurons, neurons_in_pe, doryta_id_offset,
     *        total_neurons_globally);
     */
    return to_ret;
}

struct NeuronGroupInfo layout_master_info_latest_group(void) {
    assert(num_neuron_groups > 0);
    return (struct NeuronGroupInfo) {
        .id_offset = neuron_groups[num_neuron_groups - 1].global_neuron_offset,
        .num_neurons = neuron_groups[num_neuron_groups - 1].num_neurons,
    };
}

static void master_allocate(int sizeof_neuron);
static void master_init_neurons(neuron_init_f, synapse_init_f);

void layout_master_init(int sizeof_neuron,
        neuron_init_f neuron_init, synapse_init_f synapse_init) {
    master_allocate(sizeof_neuron);
    master_init_neurons(neuron_init, synapse_init);
}


static void map_pseudo_linear(void);
static struct tw_lp * gid_to_local_lp(tw_lpid gid);

static void master_allocate(int sizeof_neuron) {
    assert(!initialized);
    synapses =
        malloc(total_neurons_in_pe * sizeof(struct SynapseCollection));
    naked_synapses =
        malloc(total_synapses * sizeof(struct Synapse));
    neurons =
        malloc(total_neurons_in_pe * sizeof(void*));
    naked_neurons =
        malloc(total_neurons_in_pe * sizeof_neuron);

    if (synapses == NULL
       || naked_synapses == NULL
       || naked_neurons == NULL
       || neurons == NULL) {
        tw_error(TW_LOC, "Not able to allocate space for neurons and synapses");
    }

    // Connecting neurons pointers to naked neuron array
    for (size_t i = 0; i < total_neurons_in_pe; i++) {
        neurons[i] =
            (void*) & naked_neurons[i*sizeof_neuron];
    }

    pe_gid_offset = g_tw_mynode * max_num_neurons_per_pe;

    // Custom Mapping
    g_tw_mapping = CUSTOM;
    g_tw_custom_initial_mapping = &map_pseudo_linear;
    g_tw_custom_lp_global_to_local_map = &gid_to_local_lp;

    // IF there are multiple LP types
    //    you should define the mapping of GID -> lptype index
    //g_tw_lp_typemap = &model_typemap;

    initialized = true;
}


struct SynapseIterator {
    size_t doryta_id;
    int n_group;
    size_t to_id;
};

static inline bool falls_id_in_interval(size_t doryta_id, int n_group) {
    return synapse_groups[n_group].from_start <= doryta_id
        && doryta_id <= synapse_groups[n_group].from_end;
}
static inline void synapse_iter_for_neuron(struct SynapseIterator * iter, size_t doryta_id) {
    // Finding first level where `doryta_id` appears in "from" interval
    int n_group;
    for (n_group = 0; n_group < num_synap_groups; n_group++) {
        if (falls_id_in_interval(doryta_id, n_group)) {
            iter->to_id = synapse_groups[n_group].to_start;
            break;
        }
    }

    iter->n_group = n_group;
    iter->doryta_id = doryta_id;
}
static inline bool synapse_iter_end(struct SynapseIterator * iter) {
    return iter->n_group == num_synap_groups;
}
static inline size_t synapse_iter_next(struct SynapseIterator * iter) {
    assert(iter->n_group < num_synap_groups);
    size_t const toret = iter->to_id;
    if (iter->to_id < synapse_groups[iter->n_group].to_end) {
        iter->to_id++;
    } else {
        // Finding next level where `doryta_id` appears in "from" interval
        int n_group = iter->n_group + 1;
        for (; n_group < num_synap_groups; n_group++) {
            if (falls_id_in_interval(iter->doryta_id, n_group)) {
                iter->to_id = synapse_groups[n_group].to_start;
                break;
            }
        }
        iter->n_group = n_group;
    }
    return toret;
}

static void master_init_neurons(neuron_init_f neuron_init, synapse_init_f synapse_init) {
    size_t neuron_counter = 0;
    size_t synapse_shift = 0;
    for (int i = 0; i < num_neuron_groups; i++) {
        size_t const neurons_in_pe = neuron_groups[i].neurons_in_pe;
        size_t const doryta_id_offset = neuron_groups[i].doryta_id_offset;
        size_t const local_id_offset = neuron_groups[i].local_id_offset;

        for (size_t j = 0; j < neurons_in_pe; j++) {
            size_t const doryta_id = doryta_id_offset + j;
            assert(neuron_counter == local_id_offset + j);

            // Initializing synapses
            struct SynapseIterator iter;
            synapse_iter_for_neuron(&iter, doryta_id);
            size_t num_synapses_neuron = 0;
            struct Synapse * synapses_neuron = &naked_synapses[synapse_shift];
            while (!synapse_iter_end(&iter)) {
                size_t const to_doryta_id = synapse_iter_next(&iter);

                synapses_neuron->doryta_id_to_send = to_doryta_id;
                synapses_neuron->gid_to_send =
                    layout_master_doryta_id_to_gid(to_doryta_id);
                if (synapse_init != NULL) {
                    synapses_neuron->weight = synapse_init(doryta_id, to_doryta_id);
                }

                /*
                 *float const weight = synapses_neuron->weight;
                 *size_t const gid_to_send = synapses_neuron->gid_to_send;
                 *printf("PE %lu : Initializing neuron %lu (gid: %lu)"
                 *       " with synapse to %lu (gid: %lu) -> weight = %f\n",
                 *        g_tw_mynode, doryta_id, layout_master_doryta_id_to_gid(doryta_id),
                 *        to_doryta_id, gid_to_send, weight);
                 */
                synapses_neuron++;
                num_synapses_neuron++;
            }

            // Connecting (collection) synapses to synapses
            synapses[neuron_counter].num = num_synapses_neuron;
            synapses[neuron_counter].synapses =
                num_synapses_neuron == 0 ? NULL : &naked_synapses[synapse_shift];

            // Initializing neurons
            if (neuron_init != NULL) {
                neuron_init(neurons[neuron_counter], doryta_id);
            }
            neuron_counter++;
            synapse_shift += num_synapses_neuron;
        }
    }
    assert(total_neurons_in_pe == neuron_counter);
    assert(total_synapses == synapse_shift);
}


void layout_master_free(void) {
    assert(initialized);
    free(naked_synapses);
    free(naked_neurons);
    free(neurons);
    free(synapses);
    initialized = false;
}


struct SettingsNeuronLP *
layout_master_configure(struct SettingsNeuronLP *settingsNeuronLP) {
    assert(initialized);
    settingsNeuronLP->neurons = neurons;
    settingsNeuronLP->synapses = synapses;
    settingsNeuronLP->gid_to_doryta_id = layout_master_gid_to_doryta_id;

    size_t total_neurons_across_all = 0;
    for (int i = 0; i < num_neuron_groups; i++) {
        total_neurons_across_all += neuron_groups[i].num_neurons;
    }
    settingsNeuronLP->num_neurons = total_neurons_across_all;
    settingsNeuronLP->num_neurons_pe = total_neurons_in_pe;
    return settingsNeuronLP;
}


size_t layout_master_total_lps_pe(void) {
    assert(initialized);
    // If this is not true, something went terribly wrong!
    assert(total_neurons_in_pe <= max_num_neurons_per_pe);
    return total_neurons_in_pe;
}


static size_t neurons_within_pe(size_t start, size_t end) {
    if (tw_nnodes() == 1) { // "micro-optimization"
        return end - start + 1;
    }

    size_t total_overlap = 0;
    // Finding overlap with all neuron intervals in this PE
    for (int i = 0; i < num_neuron_groups; i++) {
        size_t const start_i = neuron_groups[i].doryta_id_offset;
        size_t const end_i = neuron_groups[i].doryta_id_offset
            + neuron_groups[i].neurons_in_pe - 1;

        // Only checking when intervals intersect
        if (start_i <= end && start <= end_i) {
            // The line below is the same as this. There is no standard min or
            // max function in C :(
            // total_overlap += min(end, end_i) - max(start, start_i) + 1;
            total_overlap += (end < end_i ? end : end_i)
                - (start > start_i ? start : start_i) + 1;
        }
    }

    return total_overlap;
}

void layout_master_synapses_fully(size_t from_start, size_t from_end,
        size_t to_start, size_t to_end) {
    if(num_synap_groups >= MAX_SYNAPSE_GROUPS) {
        tw_error(TW_LOC, "Only up to %d synapse groups are allowed", MAX_SYNAPSE_GROUPS);
    }

    /*
     *printf("PE %lu - `from` interval [%" PRIu64 ", %" PRIu64 "]. Max neuron ID = %" PRIu64 "\n",
     *            g_tw_mynode, from_start, from_end, total_neurons_globally-1);
     *printf("PE %lu - `to` interval [%" PRIu64 ", %" PRIu64 "]. Max neuron ID = %" PRIu64 "\n",
     *            g_tw_mynode, to_start, to_end, total_neurons_globally-1);
     */

    if (from_end < from_start || from_start >= total_neurons_globally
            || from_end >= total_neurons_globally) {
        tw_error(TW_LOC, "Incorrect `from` interval [%" PRIu64 ", %" PRIu64 "]. Max neuron ID = %" PRIu64,
                from_start, from_end, total_neurons_globally-1);
    }

    if (to_end < to_start || to_start >= total_neurons_globally
            || to_end >= total_neurons_globally) {
        tw_error(TW_LOC, "Incorrect `to` interval [%" PRIu64 ", %" PRIu64 "]. Max neuron ID = %" PRIu64,
                to_start, to_end, total_neurons_globally-1);
    }

    synapse_groups[num_synap_groups] = (struct SynapseGroup) {
        .from_start = from_start,
        .from_end   = from_end,
        .to_start   = to_start,
        .to_end     = to_end,
    };
    num_synap_groups++;

    total_synapses += neurons_within_pe(from_start, from_end)
                        * (to_end - to_start + 1);
}


// ========================== IDs conversion functions ==========================

unsigned long layout_master_gid_to_pe(uint64_t gid) {
    //return (unsigned long)gid / max_num_neurons_per_pe;
    return gid / max_num_neurons_per_pe;
}


size_t local_id_to_doryta_id(size_t id) {
    // TODO: There is a possible optimization for neurons that lie in the
    // same PE. In that case, there is no reason to reconstruct the whole
    // neuron_groups structure for this PE, instead, just check for the value
    // there

    return local_id_to_doryta_id_for_pe(id, g_tw_mynode);
}

size_t local_id_to_doryta_id_for_pe(size_t id, size_t pe) {
    size_t const max_pes = tw_nnodes();
    size_t prev_num_neurons_in_pe = 0;
    size_t num_neurons_in_pe = 0;

    for (int i = 0; i < num_neuron_groups; i++) {
        size_t const num_neurons = neuron_groups[i].num_neurons;
        size_t const initial_pe = neuron_groups[i].initial_pe;
        size_t const final_pe = neuron_groups[i].final_pe;
        size_t const total_pes = neuron_groups[i].total_pes;
        size_t const global_neuron_offset = neuron_groups[i].global_neuron_offset;

        // Yes, this is the same code used when reserving space for neurons
        // in `layout_master_reserve_neurons`. The difference is that now we
        // are computing these values for a given PE
        bool const pe_inside_range = initial_pe <= final_pe ?
            initial_pe <= pe && pe <= final_pe
            : initial_pe <= pe || pe <= final_pe;
        size_t const index_within_range = initial_pe <= pe ?
            pe - initial_pe
            : pe + (max_pes - initial_pe);
        size_t const remainder_neurons = num_neurons % total_pes;
        size_t const neurons_in_pe =
            pe_inside_range ?
            num_neurons / total_pes + (index_within_range < remainder_neurons)
            : 0;

        prev_num_neurons_in_pe = num_neurons_in_pe;
        num_neurons_in_pe += neurons_in_pe;

        if (id < num_neurons_in_pe) {
            size_t const doryta_id_offset =
                global_neuron_offset
                + index_within_range * (num_neurons / total_pes)
                + (index_within_range < remainder_neurons ?
                        index_within_range : remainder_neurons);
            return doryta_id_offset + (id - prev_num_neurons_in_pe);
        }
    }
    tw_error(TW_LOC, "The given local id (%lu) for PE (%lu) is out of range. "
            "There are only %lu LPs in the PE", id, pe, g_tw_nlp);
    return -1;
}


size_t layout_master_gid_to_doryta_id(size_t gid) {
    size_t pe = layout_master_gid_to_pe(gid);
    return local_id_to_doryta_id_for_pe(gid % max_num_neurons_per_pe, pe);
}


static inline size_t get_local_offset_for_level_in_pe(size_t pe, int level) {
    assert(pe < tw_nnodes());
    assert(level < num_neuron_groups);

    size_t const max_pes = tw_nnodes();
    size_t num_neurons_in_pe = 0;

    for (int i = 0; i < level; i++) {
        size_t const num_neurons = neuron_groups[i].num_neurons;
        size_t const initial_pe = neuron_groups[i].initial_pe;
        size_t const final_pe = neuron_groups[i].final_pe;
        size_t const total_pes = neuron_groups[i].total_pes;

        // Yes, this is the same code used when reserving space for neurons
        // in `layout_master_reserve_neurons`. The difference is that now we
        // are computing these values for a given PE
        bool const pe_inside_range = initial_pe <= final_pe ?
            initial_pe <= pe && pe <= final_pe
            : initial_pe <= pe || pe <= final_pe;
        size_t const index_within_range = initial_pe <= pe ?
            pe - initial_pe
            : pe + (max_pes - initial_pe);
        size_t const remainder_neurons = num_neurons % total_pes;
        size_t const neurons_in_pe =
            pe_inside_range ?
            num_neurons / total_pes + (index_within_range < remainder_neurons)
            : 0;

        num_neurons_in_pe += neurons_in_pe;
    }

    return num_neurons_in_pe;
}


size_t layout_master_doryta_id_to_gid(size_t doryta_id) {
    assert(doryta_id < total_neurons_globally);
    int level;

    // Finding level in which DorytaID recides
    for (level = 0; level < num_neuron_groups; level++) {
        size_t const num_neurons = neuron_groups[level].num_neurons;
        size_t const global_neuron_offset = neuron_groups[level].global_neuron_offset;
        if (doryta_id < global_neuron_offset + num_neurons) {
            break;
        }
    }
    assert(level < num_neuron_groups);

    // Finding PE in which DorytaID lives
    size_t const global_neuron_offset = neuron_groups[level].global_neuron_offset;
    size_t const num_neurons = neuron_groups[level].num_neurons;
    size_t const initial_pe = neuron_groups[level].initial_pe;
    size_t const total_pes = neuron_groups[level].total_pes;

    size_t const neurons_per_pe = num_neurons / total_pes;
    size_t const remainder_neurons = num_neurons % total_pes;

    size_t const id_within_level = doryta_id - global_neuron_offset;
    size_t pe;
    size_t offset; // offset of neuron within layer in PE

    // The first `remainder_neurons` PEs will have a total of
    // `neurons_per_pe + 1`. The ones that follow only have
    // `remainder_neurons` each
    size_t const neurons_within_remainder_pes = (neurons_per_pe + 1) * remainder_neurons;
    if (id_within_level < neurons_within_remainder_pes) {
        pe = id_within_level / (neurons_per_pe + 1);
        offset = id_within_level % (neurons_per_pe + 1);
    } else {
        pe = remainder_neurons
            + (id_within_level - neurons_within_remainder_pes) / neurons_per_pe;
        offset = (id_within_level - neurons_within_remainder_pes) % neurons_per_pe;
    }

    pe = (pe + initial_pe) % tw_nnodes();

    return pe * max_num_neurons_per_pe
        + get_local_offset_for_level_in_pe(pe, level)
        + offset;
}


// ========================== HELPER / MAPPING FUNCTIONS ==========================
//
// Master defines mapping for everything in this PE. This code has been
// copied and pasted with very little alterations from linear mapping defined
// in ROSS
static void map_pseudo_linear(void) {
    tw_lpid  lpid;
    tw_kpid  kpid;
    unsigned int j;

    // may end up wasting last KP, but guaranteed each KP has == nLPs
    unsigned int nlp_per_kp = (int)ceil((double) g_tw_nlp / (double) g_tw_nkp);

    if(!nlp_per_kp) {
        tw_error(TW_LOC, "Not enough KPs defined: %d", g_tw_nkp);
    }

#if VERIFY_MAPPING
    printf("NODE %d: nlp %lld, offset %lld\n", g_tw_mynode, g_tw_nlp, pe_gid_offset);
    printf("\tPE %d\n", g_tw_pe->id);
#endif

    for(kpid = 0, lpid = 0; kpid < g_tw_nkp; kpid++) {
        tw_kp_onpe(kpid, g_tw_pe);

#if VERIFY_MAPPING
        printf("\t\tKP %d", kpid);
#endif

        for(j = 0; j < nlp_per_kp && lpid < g_tw_nlp; j++, lpid++) {
            tw_lp_onpe(lpid, g_tw_pe, pe_gid_offset+lpid);
            tw_lp_onkp(g_tw_lp[lpid], g_tw_kp[kpid]);

#if VERIFY_MAPPING
            if(0 == j % 20) {
                printf("\n\t\t\t");
            }
            printf("%lld ", lpid+pe_gid_offset);
#endif
        }

#if VERIFY_MAPPING
        printf("\n");
#endif
    }

    if(!g_tw_lp[g_tw_nlp-1]) {
        tw_error(TW_LOC, "Not all LPs defined! (g_tw_nlp=%d)", g_tw_nlp);
    }

    if(g_tw_lp[g_tw_nlp-1]->gid != pe_gid_offset + g_tw_nlp - 1) {
        tw_error(TW_LOC, "LPs not sequentially enumerated!");
    }
}


//Given a gid, return the local LP (global id => local PE mapping)
static struct tw_lp * gid_to_local_lp(tw_lpid gid) {
  int local_id = gid - pe_gid_offset;
  return g_tw_lp[local_id];
}

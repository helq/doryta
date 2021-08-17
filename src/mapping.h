#ifndef DORYTA_MAPPING_H
#define DORYTA_MAPPING_H

#include <stdint.h>

/** @file
 * The mapping for LPs to SEs in a ROSS model.
 */

// To not load ross headers
struct tw_lp;

/** Mapping of LPs to SEs.
 * Given an LP's GID (global ID) return the PE (aka node, MPI Rank)
 */
unsigned long linear_map(uint64_t gid);

uint64_t get_neuron_id(struct tw_lp *lp);

uint64_t get_gid_for_neuronid(uint64_t);

/*
void model_cutom_mapping(void);
tw_lp * model_mapping_to_lp(tw_lpid lpid);
tw_peid model_map(tw_lpid gid);
*/

#endif /* end of include guard */

#ifndef HIGHLIFE_MAPPING_H
#define HIGHLIFE_MAPPING_H

/** @file
 * The mapping for LPs to SEs in a ROSS model.
 * This file includes:
 * - the required LP GID -> PE mapping function
 * - Commented out example of LPType map (when there multiple LP types)
 * - Commented out example of one set of custom mapping functions:
 *   - setup function to place LPs and KPs on PEs
 *   - local map function to find LP in local PE's array
 */


#include <ross.h>

/** Mapping of LPs to SEs.
 * Given an LP's GID (global ID) return the PE (aka node, MPI Rank)
 */
tw_peid highlife_map(tw_lpid gid);

/*
void model_cutom_mapping(void);
tw_lp * model_mapping_to_lp(tw_lpid lpid);
tw_peid model_map(tw_lpid gid);
*/

#endif /* end of include guard */

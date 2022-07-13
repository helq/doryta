#ifndef DORYTA_SRC_MODEL_LOADERS_STRATEGY_MODES_H
#define DORYTA_SRC_MODEL_LOADERS_STRATEGY_MODES_H

#include <stdint.h>
#include <stddef.h>
#include <assert.h>

enum MODEL_LOADING_MODE {
    MODEL_LOADING_MODE_none_yet,
    MODEL_LOADING_MODE_layouts,
    MODEL_LOADING_MODE_custom
};

struct StrategyParams {
    enum MODEL_LOADING_MODE mode;
    size_t (*lps_in_pe) (void);
    unsigned long (*gid_to_pe) (uint64_t);
    unsigned long (*doryta_id_to_pe) (int32_t);
    size_t (*doryta_id_to_local_id) (int32_t);
    int32_t (*local_id_to_doryta_id) (size_t);
};

static inline void initialize_StrategyParams(struct StrategyParams * params) {
    params->mode = MODEL_LOADING_MODE_none_yet;
    params->lps_in_pe = NULL;
    params->gid_to_pe = NULL;
    params->doryta_id_to_pe = NULL;
    params->doryta_id_to_local_id = NULL;
}

static inline void assert_valid_StrategyParams(struct StrategyParams const * params) {
#ifndef NDEBUG
    if (params->mode == MODEL_LOADING_MODE_none_yet) {
        assert(params->gid_to_pe == NULL);
        assert(params->lps_in_pe == NULL);
        assert(params->doryta_id_to_pe == NULL);
        assert(params->doryta_id_to_local_id == NULL);
    } else if (params->mode != MODEL_LOADING_MODE_custom) {
        assert(params->gid_to_pe != NULL);
        assert(params->lps_in_pe != NULL);
        assert(params->doryta_id_to_pe != NULL);
        assert(params->doryta_id_to_local_id != NULL);
    }
#endif // NDEBUG
}

void set_model_strategy_mode(const struct StrategyParams *);
struct StrategyParams get_model_strategy_mode(void);

#endif /* end of include guard */

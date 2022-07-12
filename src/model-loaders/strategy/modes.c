#include "modes.h"
#include <ross.h>

static struct StrategyParams current_strategy = {
    .mode = MODEL_LOADING_MODE_none_yet,
    0
};

void set_model_strategy_mode(const struct StrategyParams * params) {
    assert_valid_StrategyParams(params);
    if (params->mode == current_strategy.mode) {
        return;
    }
    if (current_strategy.mode != MODEL_LOADING_MODE_none_yet) {
        tw_error(TW_LOC, "Trying to set a new strategy again. Strategies can be set only once");
    }

    current_strategy = *params;
}

struct StrategyParams get_model_strategy_mode(void) {
    return current_strategy;
}

#ifndef DORYTA_SRC_UTILS_MATH_H
#define DORYTA_SRC_UTILS_MATH_H

#include <stdint.h>
#include <assert.h>

static inline int32_t divceil_i32(int32_t x, int32_t y) {
    assert(y > 0);
    return x/y + (x % y > 0);
}

#endif /* end of include guard */

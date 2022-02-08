#include "pcg32_random.h"
#include <pcg_basic.h>

float pcg32_float_r(pcg32_random_t * rng) {
    // pcg32_boundedrand(n) returns a number between the interval [0, n),
    // so asking for pcg32_boundedrand(2^31 + 1) gets us the interval
    // [0, 2^31]. This means that we can divide the random number we get by
    // 2^31 and we will get a floating point number in the interval [0, 1]
    // inclusive for both
    // NOTE: 2^31 is an arbitrary number. It well could be
    // 2^23, which is the mantisa's size for single-floating point numbers
    uint32_t const num = pcg32_boundedrand_r(rng, 2147483649); // 2^31 + 1
    return num / 2147483648.0;
}

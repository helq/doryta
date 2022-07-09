#ifndef DORYTA_UTILS_IO_H
#define DORYTA_UTILS_IO_H

#include <ross.h>
#include <arpa/inet.h>
#include "endian/endian.h"

/** @file
 * Utilities that don't rely on ROSS.
 */

/** Check and create directory. */
void check_folder(char const * const path);

// ======================= READING FROM A FILE FUNCTIONALITY =======================

// Each function can produce an error
static inline uint8_t  load_uint8(FILE *fp) {
    int res = fgetc(fp);
    if (res == EOF) {
        tw_error(TW_LOC, "Input file corrupt (note: premature EOF)");
    }
    return res;
}

/* Checks if the data was correctly read. If it didn't it fires a "ross" error.
 * `ret_code` is the value returned when executing `fread`, and `count` is
 * the expected number of objects to have read, which must be equal to `ret_code`.
 */
static inline void check_if_failure(FILE * fp, size_t ret_code, size_t count) {
    if (ret_code != count) { // Failure on reading file
        char const * error_msg;
        if (feof(fp)) {
            error_msg = "Input file corrupt (note: premature EOF)";
        } else if (ferror(fp)) {
            error_msg = "Input file corrupt (note: error found while reading file)";
        } else {
            error_msg = "Unknown error when reading file";
        }
        tw_error(TW_LOC, error_msg);
    }
}
static inline uint16_t load_uint16(FILE * fp) {
    uint16_t res;
    size_t ret_code = fread(&res, sizeof(res), 1, fp);
    check_if_failure(fp, ret_code, 1);
    return be16toh(res);
}
static inline uint32_t load_uint32(FILE * fp) {
    uint32_t res;
    size_t ret_code = fread(&res, sizeof(res), 1, fp);
    check_if_failure(fp, ret_code, 1);
    return be32toh(res);
}
static inline int32_t load_int32(FILE * fp) {
    uint32_t res;
    size_t ret_code = fread(&res, sizeof(res), 1, fp);
    check_if_failure(fp, ret_code, 1);
    return be32toh(res);
}
static inline int64_t load_int64(FILE * fp) {
    uint64_t res;
    size_t ret_code = fread(&res, sizeof(res), 1, fp);
    check_if_failure(fp, ret_code, 1);
    return be64toh(res);
}
static inline float load_float(FILE * fp) {
    union {
        uint32_t ui32;
        float flt;
    } res;
    size_t ret_code = fread(&res.ui32, sizeof(res.ui32), 1, fp);
    check_if_failure(fp, ret_code, 1);
    res.ui32 = be32toh(res.ui32);
    return res.flt;
}
static inline double load_double(FILE * fp) {
    union {
        uint64_t ui64;
        double flt;
    } res;
    size_t ret_code = fread(&res.ui64, sizeof(res.ui64), 1, fp);
    check_if_failure(fp, ret_code, 1);
    res.ui64 = be64toh(res.ui64);
    return res.flt;
}
static inline void load_floats(FILE * fp, float * buffer, int32_t num) {
    size_t ret_code = fread(buffer, sizeof(float), num, fp);
    check_if_failure(fp, ret_code, num);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    for (int32_t i = 0; i < num; i++) {
        union {
            uint32_t ui32;
            float flt;
        } val;
        val.flt = buffer[i];
        val.ui32 = be32toh(val.ui32);
        buffer[i] = val.flt;
    }
#endif
}
static inline void load_int32s(FILE * fp, int32_t * buffer, int32_t num) {
    size_t ret_code = fread(buffer, sizeof(int32_t), num, fp);
    check_if_failure(fp, ret_code, num);
#if __BYTE_ORDER == __LITTLE_ENDIAN
    for (int32_t i = 0; i < num; i++) {
        buffer[i] = be32toh(buffer[i]);
    }
#endif
}
#endif /* end of include guard */

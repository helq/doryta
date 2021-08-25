// The goal of this header is to define a function with its own closure

#ifndef DORYTA_SRC_UTILS_CLOSURE_H
#define DORYTA_SRC_UTILS_CLOSURE_H

#include <doryta_config.h>
#include <stddef.h>

#define TYPE_POS _t

#define CREATE_CLOSURE_TYPE(type_name, output_t, input_t) \
    typedef output_t (* type_name##_f ) (void *, input_t); \
    typedef input_t type_name##_input_t; \
    typedef output_t type_name##_output_t; \
    typedef struct { \
        void *        closure_data; \
        type_name##_f fun; \
    } type_name

#ifdef CMAKE_C_EXTENSIONS
// Converting value of `p` to `conversion_type`, but checking
// beforehand that it matches the type `original_type`. Both types
// must be pointers!
#define type_checked(conversion_type, original_type, p) \
    ((conversion_type) (1 ? p : (original_type) 0))

// Notice that this macro requires the GNU extension `typeof` to
// statically check the type of the function passed
#define CREATE_CLOSURE(dst, type, data, fun_) \
    { \
        void * data_holder = malloc(sizeof(data)); \
        memcpy(data_holder, &data, sizeof(data)); \
        type * closure = malloc(sizeof(type)); \
        closure->closure_data = data_holder; \
        closure->fun = \
            type_checked( \
                type##_f, \
                type##_output_t (*) (typeof(data) *, type##_input_t), \
                fun_); \
        dst = closure; \
    }
#else
#define CREATE_CLOSURE(dst, type, data, fun_) \
    { \
        void * data_holder = malloc(sizeof(data)); \
        memcpy(data_holder, &data, sizeof(data)); \
        type * closure = malloc(sizeof(type)); \
        closure->closure_data = data_holder; \
        closure->fun = (type##_f) fun_; \
        dst = closure; \
    }
#endif

// This sounds like it's a stupid idea, but we want to have the
// same interface for functions and closures
#define CREATE_NON_CLOSURE(dst, type, fun_) \
    { \
        type * closure = malloc(sizeof(type)); \
        closure->closure_data = NULL; \
        closure->fun = fun_; \
        dst = closure; \
    }

#define DESTROY_CLOSURE(closure) \
    { \
        if (closure->closure_data != NULL) { \
            free(closure->closure_data); \
        } \
        free(closure); \
    }

#define call_closure(closure, input) \
    closure->fun(closure->closure_data, input)

#endif /* end of include guard */

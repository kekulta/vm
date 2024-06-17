#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef struct object_struct object_struct_t;

typedef uint16_t             flags_t;
typedef int32_t              jint_t;

typedef enum {
    J_INT,
    J_REFERENCE,
} type_t;

typedef union type_value {
    jint_t           jint;
    object_struct_t *object_struct;
} type_value_t;

#endif

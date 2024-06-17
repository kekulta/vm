#ifndef ARRAY_H
#define ARRAY_H

#include <stdint.h>
#include <stdlib.h>

typedef struct object_struct object_struct_t;

typedef enum {
    ARRAY_I,
    ARRAY_A,
} array_type_t;

typedef union array_value {
    object_struct_t *object;
    int32_t          jint;
} array_value_t;

typedef struct array_struct {
    array_type_t   type;
    /* NULL if type is not ARRAY_A */
    char          *class_name;
    size_t         dimensions;
    size_t         size;
    array_value_t *value;
} array_struct_t;

#endif

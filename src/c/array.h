#ifndef ARRAY_H
#define ARRAY_H

#include <stdint.h>
#include <stdlib.h>
#include "types.h"

typedef struct object_struct object_t;

typedef struct aarray_struct {
    size_t     size;
    object_t **value;
} aarray_t;

typedef struct iarray_struct {
    size_t  size;
    jint_t *value;
} iarray_t;
#endif

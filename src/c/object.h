#ifndef OBJECT_H
#define OBJECT_H

#include "types.h"

typedef struct aarray_struct   aarray_t;
typedef struct iarray_struct   iarray_t;
typedef struct instance_struct instance_t;

typedef union object_value {
    instance_t *instance;
    aarray_t   *aarray;
    iarray_t   *iarray;
} object_value_t;

typedef struct object_struct {
    descriptor_t   descriptor;
    object_value_t value;
} object_t;

#endif

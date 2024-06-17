#ifndef INSTANCE_H
#define INSTANCE_H

#include "jvm.h"
#include "types.h"

typedef struct field_struct {
    char        *field_name;
    descriptor_t descriptor;
    value_t     value;
} field_t;

typedef struct instance_struct {
    char    *class_name;
    size_t   fields_count;
    field_t *fields;
} instance_t;

#endif

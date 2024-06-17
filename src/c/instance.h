#ifndef INSTANCE_H
#define INSTANCE_H

#include "jvm.h"

typedef struct field_struct {
    char        *class_name;
    char        *field_name;
    type_t       type;
    char        *descriptor;
    type_value_t value;
} field_struct_t;

typedef struct instance_struct {
    char           *class_name;
    size_t          fields_count;
    field_struct_t *fields;
} instance_struct_t;

bool is_instance(vm_context_t *context, class_struct_t *super_class,
                 instance_struct_t *instance);

#endif

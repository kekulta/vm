#ifndef OBJECT_H
#define OBJECT_H

typedef struct instance_struct instance_struct_t;
typedef struct array_struct    array_struct_t;

typedef enum {
    OBJ_INSTANCE,
    OBJ_ARRAY,
} object_type_t;

typedef union object_value {
    instance_struct_t *instance;
    array_struct_t    *array;
} object_value_t;

typedef struct object_struct {
    object_type_t  type;
    object_value_t value;
} object_struct_t;

#endif

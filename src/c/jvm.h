#ifndef JVM_H
#define JVM_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "array.h"
#include "types.h"

typedef struct class_struct  class_struct_t;
typedef struct object_struct object_struct_t;
typedef struct method_info   method_info_t;

typedef struct vm_value {
    type_t       type;
    type_value_t value;
} vm_value_t;

typedef struct vm_frame {
    class_struct_t *class_struct;
    char           *descriptor;
    size_t          stack_count;
    size_t          max_stack;
    vm_value_t     *stack;
    size_t          max_locals;
    vm_value_t     *locals;
    size_t          pc;
    size_t          code_length;
    uint8_t        *code;
    bool            is_native;
    char           *method_name;
} vm_frame_t;

typedef struct frame_stack {
    size_t       size;
    size_t       count;
    vm_frame_t **frames;
} frame_stack_t;

typedef struct class_table {
    size_t           size;
    size_t           count;
    class_struct_t **class_pool;
} class_table_t;

typedef struct object_table {
    size_t            size;
    size_t            count;
    object_struct_t **object_pool;
} object_table_t;

typedef struct vm_context {
    size_t          classpath_size;
    char          **classpath;
    class_table_t  *class_table;
    object_table_t *object_table;
    frame_stack_t  *frame_stack;
} vm_context_t;

class_struct_t  *find_class(vm_context_t *context, char *class_name);

object_struct_t *allocate_object(object_table_t *object_table);

object_struct_t *allocate_instance(vm_context_t   *context,
                                   class_struct_t *class_struct,
                                   bool            is_static);

object_struct_t *allocate_array_i(vm_context_t *context, size_t dimensions_num,
                                  size_t *dimensions_sizes);

object_struct_t *allocate_array_a(vm_context_t   *context,
                                  class_struct_t *class_struct,
                                  size_t          dimensions_num,
                                  size_t         *dimensions_sizes);

bool             format_class_name(char *dest, char *class_name);

bool format_class_path(char *dest, char *classpath, char *class_name);

bool check_flags(flags_t acc_flags, flags_t t_acc_flags, flags_t f_acc_flags);

bool push_frame(vm_context_t *context, vm_frame_t *frame);

bool pop_frame(vm_context_t *context);

vm_frame_t *construct_frame(vm_context_t *context, class_struct_t *class_struct,
                            method_info_t *method, vm_value_t *args);

bool        set_local(vm_frame_t *frame, size_t num, vm_value_t value);

char       *extract_argument(char *dest, char *descriptor);

char       *extract_return(char *dest, char *descriptor);

size_t      count_dimensions(char *descriptor);

array_type_t extract_array_type(char *descriptor);

void         extract_class_name(char *dest, char *descriptor);

size_t       count_method_args(char *descriptor);

bool check_value_instance(vm_context_t *context, class_struct_t *class_struct,
                          vm_value_t value, bool can_be_null);

bool check_value_array_a(vm_context_t *context, class_struct_t *class_struct,
                         vm_value_t value, bool can_be_null, size_t dimensions);

bool check_value_array_i(vm_value_t value, size_t dimensions, bool can_be_null);

#endif

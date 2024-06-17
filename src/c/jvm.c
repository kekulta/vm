#include <stdio.h>
#include <string.h>
#include "array.h"
#include "class.h"
#include "instance.h"
#include "jvm.h"
#include "object.h"
#include "types.h"

bool
set_local(frame_t *frame, size_t num, vm_value_t value)
{
    if (num >= frame->max_locals) {
        return false;
    }

    frame->locals[num] = value;

    return true;
}

char *
extract_argument(char *dest, char *descriptor)
{
    size_t offset;
    char  *start;

    offset = 0;
    start = descriptor;

    if (start[0] == '(') {
        start++;
    }

    if (start[0] == ')' || start[0] == '\0') {
        return NULL;
    }

    while (1) {
        switch (start[offset]) {
            case 'I':
                if (dest != NULL) {
                    strncpy(dest, start, offset + 1);
                }
                return start + offset + 1;
            case '[': offset++; break;
            case 'L':
                while (start[offset] != ';') {
                    offset++;
                }
                if (dest != NULL) {
                    strncpy(dest, start, offset + 1);
                }
                return start + offset + 1;
            default: return NULL;
        }
    }
}

char *
extract_return(char *dest, char *descriptor)
{
    while (descriptor[0] != ')') {
        descriptor++;
    }

    descriptor++;

    return descriptor;
}

size_t
count_method_args(char *descriptor)
{
    size_t count;
    char  *temp;

    count = 0;
    temp = descriptor;
    while (1) {
        if ((temp = extract_argument(NULL, temp)) != NULL) {
            count++;
        } else {
            break;
        }
    }

    return count;
}

class_t *
find_class(vm_context_t *context, char *class_name)
{
    for (size_t i = 0; i < context->class_table->count; i++) {
        if (!strcmp(context->class_table->class_pool[i]->this_class,
                    class_name)) {
            return context->class_table->class_pool[i];
        }
    }

    return NULL;
}

object_t *
allocate_object(vm_context_t *context)
{
    size_t new_size;

    if (context->object_table->count == context->object_table->size) {

        if (context->object_table->size == 0) {
            new_size = 10;
        } else {
            new_size = context->object_table->size * 2;
        }

        context->object_table->size = new_size;
        context->object_table->object_pool = realloc(
            context->object_table->object_pool, sizeof(object_t *) * new_size);
    }

    return context->object_table->object_pool[context->object_table->count++] =
               malloc(sizeof(object_t));
}

object_t *
allocate_array_i(vm_context_t *context, size_t size)
{
    object_t *new_object;
    iarray_t *new_array;

    new_array = malloc(sizeof(iarray_t));
    new_array->size = size;
    new_array->value = calloc(new_array->size, sizeof(jint_t));

    new_object = allocate_object(context);
    new_object->descriptor = "[I";
    new_object->value.iarray = new_array;

    return new_object;
}

object_t *
allocate_array_a(vm_context_t *context, descriptor_t descriptor,
                 size_t dimensions_num, size_t *dimensions_sizes)
{
    object_t *new_object;
    aarray_t *new_array;

    new_array = malloc(sizeof(aarray_t));
    new_array->size = dimensions_sizes[0];
    new_array->value = calloc(new_array->size, sizeof(object_t *));

    if (dimensions_num > 1) {
        for (size_t i = 0; i < new_array->size; i++) {
            new_array->value[i] = allocate_array_a(context, descriptor + 1,
                                                   dimensions_num - 1,
                                                   dimensions_sizes + 1);
        }
    }

    new_object = allocate_object(context);
    new_object->descriptor = descriptor;
    new_object->value.aarray = new_array;

    return new_object;
}

object_t *
allocate_instance(vm_context_t *context, class_t *class_struct, bool is_static)
{
    flags_t       t_acc_flags, f_acc_flags;
    size_t        curr_field;
    char         *desciptor;
    field_info_t *field_info;
    object_t     *new_object;
    instance_t   *new_instance;
    field_t      *new_field;

    if (is_static) {
        t_acc_flags = ACC_FIELD_STATIC;
        f_acc_flags = 0;
    } else {
        t_acc_flags = 0;
        f_acc_flags = ACC_FIELD_STATIC;
    }

    new_object = allocate_object(context);

    new_instance = malloc(sizeof(instance_t));
    new_object->descriptor = class_struct->descriptor;
    new_object->value.instance = new_instance;

    if (is_static) {
        class_struct->static_instance = new_object;
    }

    new_instance->class_name = class_struct->this_class;
    new_instance->fields_count = count_fields(context, class_struct,
                                              t_acc_flags, f_acc_flags);
    new_instance->fields = malloc(sizeof(field_t) * new_instance->fields_count);

    curr_field = 0;

    while (1) {
        for (size_t i = 0; i < class_struct->fields_count; i++) {
            field_info = class_struct->fields + i;

            if (!check_flags(field_info->access_flags, t_acc_flags,
                             f_acc_flags)) {
                continue;
            }
            new_instance->fields[curr_field].field_name = field_info->name;
            new_instance->fields[curr_field].descriptor =
                field_info->descriptor;
            new_instance->fields[curr_field].value.object = NULL;
            curr_field++;
        }

        if (class_struct->super_class == NULL) {
            break;
        }
        class_struct = find_class(context, class_struct->super_class);
    }

    return new_object;
}

bool
format_class_name(char *dest, char *class_name)
{
    size_t i;

    if (dest != class_name) {
        strcpy(dest, class_name);
    }

    i = 0;
    while (dest[i] != '\0') {
        if (dest[i] == '.') {
            dest[i] = '/';
        }
        i++;
    }

    return true;
}

bool
format_class_path(char *dest, char *classpath, char *class_name)
{
    size_t classpath_len;

    classpath_len = strlen(classpath);
    strcpy(dest, classpath);
    while (classpath_len > 0 && dest[classpath_len - 1] == '/') {
        dest[classpath_len - 1] = '\0';
        classpath_len--;
    }
    dest[classpath_len] = '/';
    classpath_len++;
    strcpy(dest + classpath_len, class_name);

    format_class_name(dest + classpath_len, dest + classpath_len);
    strcpy(dest + strlen(dest), ".class");
    return true;
}

bool
check_flags(flags_t acc_flags, flags_t t_acc_flags, flags_t f_acc_flags)
{
    return (acc_flags & t_acc_flags) == t_acc_flags
           && ((~acc_flags) & f_acc_flags) == f_acc_flags;
}

bool
push_frame(vm_context_t *context, frame_t *frame)
{
    size_t new_size;

    if (context->frame_stack->count == context->frame_stack->size) {

        if (context->frame_stack->size == 0) {
            new_size = 10;
        } else {
            new_size = context->frame_stack->size * 2;
        }

        context->frame_stack->size = new_size;
        context->frame_stack->frames = realloc(context->frame_stack->frames,
                                               sizeof(frame_t *) * new_size);
    }

    context->frame_stack->frames[context->frame_stack->count++] = frame;
    return true;
}

bool
pop_frame(vm_context_t *context)
{
    frame_t *frame;
    size_t   new_count;

    if (context->frame_stack->count == 0) {
        return false;
    }
    new_count = --context->frame_stack->count;

    frame = context->frame_stack->frames[new_count];
    printf("frame: %s\n", frame->frame_class->this_class);

    free(frame->locals);
    free(frame->stack);
    free(frame);

    context->frame_stack->frames[new_count] = NULL;

    return true;
}

char *
allocate_class_name(char *descriptor)
{
    size_t str_size;
    char  *class_name;

    if (descriptor[0] == 'L') {
        descriptor++;
    } else {
        while (descriptor[0] == '[') {
            descriptor++;
        }

        if (descriptor[0] == 'L') {
            descriptor++;
        }
    }

    str_size = strlen(descriptor);
    class_name = malloc(sizeof(char) * (str_size - 1));

    strncpy(class_name, descriptor, str_size - 1);

    return class_name;
}

bool
is_subclass_by_desc(vm_context_t *context, descriptor_t super_class_desc,
                    descriptor_t subclass_desc)
{
    char    *super_class_name, *subclass_name;
    class_t *super_class;
    bool     result;

    super_class_name = allocate_class_name(super_class_desc);
    subclass_name = allocate_class_name(subclass_desc);

    result = is_subclass(context, super_class_name, subclass_name);

    free(super_class_name);
    free(subclass_name);
    return result;
}

descriptor_t
extract_array_type(descriptor_t descriptor)
{
    while (descriptor[0] == '[') {
        descriptor++;
    }

    return descriptor;
}

size_t
count_dimensions(descriptor_t descriptor)
{
    size_t count;

    count = 0;
    while (descriptor[0] == '[') {
        count++;
        descriptor++;
    }

    return count;
}

bool
check_assignable(vm_context_t *context, descriptor_t destination,
                 descriptor_t value)
{
    descriptor_t dest_type, val_type;

    if (strchr(destination, '(') != NULL || strchr("value", '(') != NULL) {
        return false;
    }

    if (strcmp(destination, value) == 0) {
        return true;
    }

    if (count_dimensions(destination) != count_dimensions(value)) {
        return false;
    }

    dest_type = extract_array_type(destination);
    val_type = extract_array_type(value);

    if (dest_type[0] != 'L' || val_type[0] != 'L') {
        return dest_type[0] == val_type[0];
    }

    return is_subclass_by_desc(context, destination, value);
}

frame_t *
construct_frame(vm_context_t *context, class_t *frame_class,
                method_info_t *frame_method, vm_value_t *args)
{
    /* TODO: stop leaking memory on typechecking fail */

    frame_t          *this_frame;
    attribute_code_t *code;
    size_t            arg_num;
    char             *arg_desc, *method_desc;

    this_frame = calloc(1, sizeof(frame_t));
    this_frame->frame_class = frame_class;
    this_frame->method_name = frame_method->name;
    this_frame->descriptor = frame_method->descriptor;
    this_frame->stack_count = 0;

    if (!(frame_method->access_flags & ACC_METHOD_NATIVE)) {
        code = find_code(frame_method);
        this_frame->max_stack = code->max_stack;
        this_frame->max_locals = code->max_locals;
        this_frame->code_length = code->code_length;
        this_frame->code = code->code;
        this_frame->is_native = false;
    } else {
        this_frame->is_native = true;
        this_frame->code = NULL;
        this_frame->max_locals = count_method_args(frame_method->descriptor);
    }

    this_frame->pc = 0;
    this_frame->stack = calloc(this_frame->max_stack, sizeof(vm_value_t));
    this_frame->locals = calloc(this_frame->max_locals, sizeof(vm_value_t));

    arg_num = 0;

    if (check_flags(frame_method->access_flags, NO_FLAGS, ACC_METHOD_STATIC)) {
        if (!check_assignable(context, frame_class->descriptor,
                              args[arg_num].descriptor)) {
            fprintf(
                stderr,
                "Error while invoking virtual method, no instance provided.\n");
            return NULL;
        }
        set_local(this_frame, arg_num, args[arg_num]);
        arg_num++;
    }

    arg_desc = malloc(sizeof(char) * strlen(frame_method->descriptor));
    method_desc = frame_method->descriptor;

    while (1) {
        if ((method_desc = extract_argument(arg_desc, method_desc)) == NULL) {
            break;
        }

        if (check_assignable(context, arg_desc, args[arg_num].descriptor)) {
            set_local(this_frame, arg_num, args[arg_num]);
        } else {
            fprintf(
                stderr,
                "Error while invoking %s.%s. Required %s, but %sprovided.\n",
                this_frame->frame_class->this_class, this_frame->method_name,
                arg_desc, args[arg_num].descriptor);
            return NULL;
        }
    }

    free(arg_desc);
    return this_frame;
}

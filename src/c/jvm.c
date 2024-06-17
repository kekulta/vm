#include <stdio.h>
#include <string.h>
#include "array.h"
#include "boot_loader.h"
#include "class.h"
#include "instance.h"
#include "jvm.h"
#include "object.h"

bool
set_local(vm_frame_t *frame, size_t num, vm_value_t value)
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
count_dimensions(char *descriptor)
{
    size_t count;

    count = 0;
    while (descriptor[0] == '[') {
        count++;
        descriptor++;
    }

    return count;
}

array_type_t
extract_array_type(char *descriptor)
{
    while (descriptor[0] == '[') {
        descriptor++;
    }

    /* TODO: UNSAFE EXIT */
    switch (descriptor[0]) {
        case 'I': return ARRAY_I;
        case 'L': return ARRAY_A;
        default: printf("UNKNOWN DESCRIPTOR: %s", descriptor); exit(1);
    }
}

void
extract_class_name(char *dest, char *descriptor)
{
    size_t str_size;

    if (descriptor[0] == 'L') {
        descriptor++;
    }

    while (descriptor[0] == '[') {
        descriptor++;
    }

    if (descriptor[0] == 'L') {
        descriptor++;
    }

    strcpy(dest, descriptor);
    str_size = strlen(dest);

    if (dest[str_size - 1] == ';') {
        dest[str_size - 1] = '\0';
    }
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

bool
check_value_instance(vm_context_t *context, class_struct_t *class_struct,
                     vm_value_t value, bool can_be_null)
{
    if (value.type != J_REFERENCE) {
        return false;
    }

    if (value.value.object_struct == NULL) {
        return can_be_null;
    }

    return value.value.object_struct->type == OBJ_INSTANCE
           && is_instance(context, class_struct,
                          value.value.object_struct->value.instance);
}

bool
check_value_array_a(vm_context_t *context, class_struct_t *class_struct,
                    vm_value_t value, bool can_be_null, size_t dimensions)
{
    if (value.type != J_REFERENCE) {
        return false;
    }

    if (value.value.object_struct == NULL) {
        return can_be_null;
    }

    if (value.value.object_struct->type != OBJ_ARRAY
        || value.value.object_struct->value.array->type != ARRAY_A) {
        return false;
    }

    return is_subclass(context, class_struct,
                       value.value.object_struct->value.array->class_name)
           && value.value.object_struct->value.array->dimensions == dimensions;
}

bool
check_value_array_i(vm_value_t value, size_t dimensions, bool can_be_null)
{
    if (value.type != J_REFERENCE) {
        return false;
    }

    if (value.value.object_struct == NULL) {
        return can_be_null;
    }
    return value.type == J_REFERENCE && value.value.object_struct != NULL
           && value.value.object_struct->type == OBJ_ARRAY
           && value.value.object_struct->value.array->type == ARRAY_I
           && value.value.object_struct->value.array->dimensions == dimensions;
}

class_struct_t *
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

object_struct_t *
allocate_object(object_table_t *object_table)
{
    size_t new_size;

    if (object_table->count == object_table->size) {

        if (object_table->size == 0) {
            new_size = 10;
        } else {
            new_size = object_table->size * 2;
        }

        object_table->size = new_size;
        object_table->object_pool = realloc(
            object_table->object_pool, sizeof(object_struct_t *) * new_size);
    }

    return object_table->object_pool[object_table->count++] = malloc(
               sizeof(object_struct_t));
}

object_struct_t *
allocate_array_i(vm_context_t *context, size_t dimensions_num,
                 size_t *dimensions_sizes)
{
    object_struct_t *new_object;
    array_struct_t  *new_array;

    new_array = malloc(sizeof(array_struct_t));
    new_array->type = ARRAY_I;
    new_array->class_name = "I";
    new_array->dimensions = dimensions_num;
    new_array->size = dimensions_sizes[0];
    new_array->value = malloc(sizeof(array_value_t) * new_array->size);

    if (dimensions_num == 1) {
        for (size_t i = 0; i < new_array->size; i++) {
            new_array->value[i].jint = 0;
        }
    } else {
        for (size_t i = 0; i < new_array->size; i++) {
            new_array->value[i].object = allocate_array_i(
                context, dimensions_num - 1, dimensions_sizes + 1);
        }
    }

    new_object = allocate_object(context->object_table);
    new_object->type = OBJ_ARRAY;
    new_object->value.array = new_array;

    return new_object;
}

object_struct_t *
allocate_array_a(vm_context_t *context, class_struct_t *class_struct,
                 size_t dimensions_num, size_t *dimensions_sizes)
{
    object_struct_t *new_object;
    array_struct_t  *new_array;

    new_array = malloc(sizeof(array_struct_t));
    new_array->type = ARRAY_A;
    new_array->class_name = class_struct->this_class;
    new_array->dimensions = dimensions_num;
    new_array->size = dimensions_sizes[0];
    new_array->value = malloc(sizeof(array_value_t) * new_array->size);
    if (dimensions_num == 1) {
        for (size_t i = 0; i < new_array->size; i++) {
            new_array->value[i].object = NULL;
        }
    } else {
        for (size_t i = 0; i < new_array->size; i++) {
            new_array->value[i].object = allocate_array_a(context, class_struct,
                                                          dimensions_num - 1,
                                                          dimensions_sizes + 1);
        }
    }

    new_object = allocate_object(context->object_table);
    new_object->type = OBJ_ARRAY;
    new_object->value.array = new_array;

    return new_object;
}

object_struct_t *
allocate_instance(vm_context_t *context, class_struct_t *class_struct,
                  bool is_static)
{
    flags_t            t_acc_flags, f_acc_flags;
    size_t             curr_field;
    char              *desciptor;
    field_info_t      *field_info;
    object_struct_t   *new_object;
    instance_struct_t *new_instance;
    field_struct_t    *new_field;

    if (is_static) {
        t_acc_flags = ACC_FIELD_STATIC;
        f_acc_flags = 0;
    } else {
        t_acc_flags = 0;
        f_acc_flags = ACC_FIELD_STATIC;
    }

    new_object = allocate_object(context->object_table);
    new_instance = malloc(sizeof(instance_struct_t));
    new_object->type = OBJ_INSTANCE;
    new_object->value.instance = new_instance;
    context->object_table->object_pool[context->object_table->count] =
        new_object;
    if (is_static) {
        class_struct->static_instance = new_object;
    }
    new_instance->class_name = class_struct->this_class;
    new_instance->fields_count = count_fields(context, class_struct,
                                              t_acc_flags, f_acc_flags);

    new_instance->fields = malloc(sizeof(field_struct_t)
                                  * new_instance->fields_count);

    curr_field = 0;

    while (1) {
        for (size_t i = 0; i < class_struct->fields_count; i++) {
            field_info = class_struct->fields + i;

            if (!check_flags(field_info->access_flags, t_acc_flags,
                             f_acc_flags)) {
                continue;
            }
            new_instance->fields[curr_field].class_name =
                class_struct->this_class;
            new_instance->fields[curr_field].field_name = field_info->name;
            new_instance->fields[curr_field].descriptor =
                field_info->descriptor;
            switch (field_info->descriptor[0]) {
                case '[':
                case 'L':
                    new_instance->fields[curr_field].type = J_REFERENCE;
                    new_instance->fields[curr_field].value.object_struct = NULL;
                    break;
                case 'I':
                    new_instance->fields[curr_field].type = J_INT;
                    new_instance->fields[curr_field].value.jint = 0;
                    break;
                default:
                    printf("UNKNOWN DESCRIPTOR: %s\n", field_info->descriptor);
            }
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
push_frame(vm_context_t *context, vm_frame_t *frame)
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
                                               sizeof(vm_frame_t *) * new_size);
    }

    context->frame_stack->frames[context->frame_stack->count++] = frame;
    return true;
}

bool
pop_frame(vm_context_t *context)
{
    vm_frame_t *frame;
    size_t      new_count;

    if (context->frame_stack->count == 0) {
        return false;
    }
    new_count = --context->frame_stack->count;

    frame = context->frame_stack->frames[new_count];
    printf("frame: %s\n", frame->class_struct->this_class);

    free(frame->locals);
    free(frame->stack);
    free(frame);

    context->frame_stack->frames[new_count] = NULL;

    return true;
}

vm_frame_t *
construct_frame(vm_context_t *context, class_struct_t *class_struct,
                method_info_t *method, vm_value_t *args)
{
    /* TODO: stop leaking memory on typechecking fail */

    vm_frame_t       *frame;
    attribute_code_t *code;
    size_t            arg_num;
    char             *type, *class_name, *descriptor;
    class_struct_t   *argument_class;

    code = find_code(method);

    frame = calloc(1, sizeof(vm_frame_t));
    frame->class_struct = class_struct;
    frame->method_name = method->name;
    frame->descriptor = method->descriptor;
    frame->stack_count = 0;

    if (!(method->access_flags & ACC_METHOD_NATIVE)) {
        frame->max_stack = code->max_stack;
        frame->max_locals = code->max_locals;
        frame->code_length = code->code_length;
        frame->code = code->code;
        frame->is_native = false;
    } else {
        frame->is_native = true;
        frame->code = NULL;
        frame->max_locals = count_method_args(method->descriptor);
    }

    frame->pc = 0;
    frame->stack = calloc(frame->max_stack, sizeof(vm_value_t));
    frame->locals = calloc(frame->max_locals, sizeof(vm_value_t));

    arg_num = 0;

    if (check_flags(method->access_flags, NO_FLAGS, ACC_METHOD_STATIC)) {
        if (!check_value_instance(context, class_struct, args[arg_num],
                                  false)) {
            return NULL;
        }
        set_local(frame, arg_num, args[arg_num]);
        arg_num++;
    }

    type = malloc(sizeof(char) * strlen(method->descriptor));
    class_name = malloc(sizeof(char) * strlen(method->descriptor));
    descriptor = method->descriptor;

    while (1) {
        if ((descriptor = extract_argument(type, descriptor)) == NULL) {
            break;
        }

        switch (type[0]) {
            case 'I':
                if (args[arg_num].type == J_INT) {
                    set_local(frame, arg_num, args[arg_num]);
                    arg_num++;
                } else {
                    return NULL;
                }
                break;
            case 'L':
                extract_class_name(class_name, type);
                argument_class = resolve_class(context, class_name);
                if (check_value_instance(context, argument_class, args[arg_num],
                                         true)) {
                    set_local(frame, arg_num, args[arg_num]);
                    arg_num++;
                } else {
                    return NULL;
                }
                break;
            case '[':
                switch (extract_array_type(type)) {
                    case ARRAY_I:
                        if (check_value_array_i(args[arg_num],
                                                count_dimensions(type), true)) {
                            set_local(frame, arg_num, args[arg_num]);
                            arg_num++;
                        } else {
                            return NULL;
                        }
                        break;
                    case ARRAY_A:
                        extract_class_name(class_name, type);
                        if (check_value_array_a(
                                context, resolve_class(context, class_name),
                                args[arg_num], count_dimensions(type), true)) {
                            set_local(frame, arg_num, args[arg_num]);
                            arg_num++;
                        } else {
                            return NULL;
                        }
                        break;
                    default: return NULL;
                }
                break;
            default: return NULL;
        }
    }

    free(type);
    free(class_name);
    return frame;
}

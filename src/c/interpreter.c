#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "array.h"
#include "boot_loader.h"
#include "class_printer.h"
#include "instance.h"
#include "interpreter.h"
#include "jvm.h"
#include "object.h"
#include "types.h"

field_t *
find_field(instance_t *instance_struct, char *name, descriptor_t descriptor)
{
    field_t *field;

    field = NULL;
    for (size_t i = 0; i < instance_struct->fields_count; i++) {
        printf("%s %s -- %s %s\n", name, instance_struct->fields[i].field_name,
               descriptor, instance_struct->fields[i].descriptor);
        if (strcmp(name, instance_struct->fields[i].field_name) == 0
            && strcmp(descriptor, instance_struct->fields[i].descriptor) == 0) {
            field = instance_struct->fields + i;
            break;
        }
    }

    return field;
}

object_t *
allocate_string(vm_context_t *context, char *string)
{
    class_t    *string_class;
    object_t   *string_object, *array_object;
    instance_t *string_instance;
    iarray_t   *array;
    field_t    *field;
    size_t      length;

    length = strlen(string);
    array_object = allocate_array_i(context, length);
    array = array_object->value.iarray;

    for (size_t i = 0; i < length; i++) {
        array->value[i] = string[i];
    }

    string_class = resolve_class(context, "java/lang/String");
    string_object = allocate_instance(context, string_class, false);
    string_instance = string_object->value.instance;
    field = find_field(string_instance, "value", "[I");
    printf("%s\n", array_object->descriptor);
    field->value.object = array_object;

    return string_object;
}

bool
init_static(vm_context_t *context, class_t *class_struct)
{
    if (class_struct->static_instance != NULL) {
        return false;
    }

    class_struct->static_instance = allocate_instance(context, class_struct,
                                                      true);
    return true;
}

frame_t *
curr_frame(vm_context_t *context)
{
    return context->frame_stack->frames[context->frame_stack->count - 1];
}

bool
push_stack(frame_t *frame, vm_value_t value)
{
    if (frame->max_stack == frame->stack_count) {
        return false;
    }

    frame->stack[frame->stack_count++] = value;

    return true;
}

vm_value_t
pop_stack(frame_t *frame)
{
    vm_value_t value;
    if (frame->stack_count == 0) {
        exit(1);
    }

    value = frame->stack[--frame->stack_count];
    return value;
}

size_t
package_len(char *class_name)
{
    size_t package_len, len;
    len = strlen(class_name);
    for (size_t i = 0; i < len; i++) {
        if (class_name[i] == '/') {
            package_len = i + 1;
        }
    }

    return package_len;
}

bool
is_same_package(class_t *class_first, class_t *class_second)
{
    size_t package_len_first, package_len_second;
    bool   is_same_package;

    package_len_first = package_len(class_first->this_class);
    package_len_second = package_len(class_second->this_class);
    if (package_len_second != package_len_first) {
        is_same_package = false;
    } else {
        is_same_package = strncmp(class_first->this_class,
                                  class_second->this_class, package_len_first);
    }

    return is_same_package;
}

bool
is_accessible(vm_context_t *context, class_t *class_destination,
              class_t *class_origin, flags_t access_flags)
{

    bool same_package;

    if (strcmp(class_destination->this_class, class_origin->this_class) == 0) {
        return true;
    }
    /* TODO: FIX FLAGS */
    if (access_flags & ACC_METHOD_PRIVATE) {
        return false;
    }

    same_package = is_same_package(class_destination, class_origin);
    if (same_package) {
        return true;
    }

    if (!(access_flags & ACC_METHOD_PROTECTED)) {
        return false;
    }

    return is_subclass(context, class_destination->this_class,
                       class_origin->this_class);
}

jint_t *
get_string_data(vm_value_t string)
{
    instance_t *string_instance;
    field_t    *field;

    string_instance = string.content.object->value.instance;

    field = find_field(string_instance, "value", "[I");
    return field->value.object->value.iarray->value;
}

size_t
get_string_len(vm_value_t string)
{
    instance_t *string_instance;
    field_t    *field;

    string_instance = string.content.object->value.instance;

    field = find_field(string_instance, "value", "[I");
    return field->value.object->value.iarray->size;
}

bool
execute_invokestatic(vm_context_t *context)
{
    size_t         method_position, args_count;
    frame_t       *frame;
    method_info_t *method_info;
    char          *class_name, *descriptor, *method_name;
    method_info_t *method;
    class_t       *class_struct;
    vm_value_t    *args;
    frame_t       *new_frame;
    bool           is_method_accessible;

    /* Do *a lot* of typechecking here */
    frame = curr_frame(context);
    method_position = frame->code[frame->pc + 1] << 8
                      | frame->code[frame->pc + 2];
    class_name = get_method_class(frame->frame_class, method_position);
    method_name = get_method_name(frame->frame_class, method_position);
    descriptor = get_method_descriptor(frame->frame_class, method_position);
    class_struct = resolve_class(context, class_name);
    method = find_method_virtual(class_struct, method_name, descriptor);
    is_method_accessible = is_accessible(
        context, class_struct, frame->frame_class, method->access_flags);

    args_count = count_method_args(method->descriptor);
    args = frame->stack + frame->stack_count - args_count;

    init_static(context, class_struct);
    new_frame = construct_frame(context, class_struct, method, args);
    push_frame(context, new_frame);

    for (size_t i = 0; i < args_count; i++) {
        pop_stack(frame);
    }

    frame->pc += 3;

    return true;
}

bool
execute_aload(vm_context_t *context, size_t index, bool is_const)
{
    frame_t *frame;
    bool     result;

    frame = curr_frame(context);
    if (frame->max_locals <= index || frame->max_stack == frame->stack_count) {
        return false;
    }

    push_stack(frame, frame->locals[index]);
    if (is_const) {
        frame->pc += 1;
    } else {
        frame->pc += 2;
    }
    return true;
}

bool
execute_ldc(vm_context_t *context)
{
    frame_t   *frame;
    cp_info_t *constant;
    uint8_t    position;
    vm_value_t value;
    bool       result;

    frame = curr_frame(context);
    position = frame->code[frame->pc + 1];

    constant = frame->frame_class->constant_pool + position - 1;
    switch (constant->tag) {
        case CONSTANT_STRING:
            value.content.object = allocate_string(
                context, get_string(frame->frame_class, position));
            value.descriptor = "Ljava/lang/String;";
            push_stack(frame, value);
            result = true;
            break;
        default:
            printf("Can't load constant of type: %d\n", constant->tag);
            result = false;
            break;
    }

    if (result) {
        frame->pc += 2;
    }

    return result;
}

bool
execute_frame(vm_context_t *context)
{
    frame_t *frame;
    jint_t  *string_data;
    size_t   string_len;

    frame = curr_frame(context);

    printf("Executing frame: %s %s\n", frame->descriptor, frame->method_name);

    if (frame->is_native) {
        if (strcmp(frame->descriptor, "(Ljava/lang/String;)V") == 0
            && strcmp(frame->method_name, "nativePrint")
            && strcmp(frame->frame_class->this_class, "BuilinPrinter")) {

            string_data = get_string_data(frame->locals[0]);
            string_len = get_string_len(frame->locals[0]);
            for (size_t i = 0; i < string_len; i++) {
                printf("%c", string_data[i]);
            }

            printf("\n");
            return true;
        } else {
            printf("Unknown native method");
            exit(0);
        }
    }

    while (frame->pc < frame->code_length) {

        switch (frame->code[frame->pc]) {
            case op_ldc:
                if (!execute_ldc(context)) {
                    printf("Error while executing ldc opcode\n");
                    exit(1);
                }
                break;
            case op_invokestatic:
                if (execute_invokestatic(context)) {
                    return false;
                } else {
                    printf("Error while executing invokestatic opcode\n");
                    exit(1);
                }
                break;
            case op_aload_0: execute_aload(context, 0, true); break;
            case op_return: return true;
            default:
                print_object_table(context->object_table);
                printf("Unknown opcode: %x\n", frame->code[frame->pc]);
                exit(1);
        }
    }

    return true;
}

bool
start_interpreting(vm_context_t *context)
{
    while (context->frame_stack->count != 0) {
        if (execute_frame(context)) {
            pop_frame(context);
        }
    }

    return true;
}

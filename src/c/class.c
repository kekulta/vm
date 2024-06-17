#include "class.h"
#include <stdbool.h>
#include <string.h>
#include "boot_loader.h"
#include "jvm.h"

char *
get_class_name(class_t *class_struct, size_t position)
{
    class_info_t *class_info;

    if (position == 0) {
        return NULL;
    }

    class_info = class_struct->constant_pool[position - 1].info;
    return get_utf8(class_struct, class_info->name_index);
}

char *
get_string(class_t *class_struct, size_t position)
{
    return get_utf8(
        class_struct,
        ((string_info_t *)class_struct->constant_pool[position - 1].info)
            ->string_index);
}

char *
get_method_name(class_t *class_struct, size_t position)
{

    return get_utf8(
        class_struct,
        ((name_and_type_info_t *)class_struct
             ->constant_pool
                 [((ref_info_t *)class_struct->constant_pool[position - 1].info)
                      ->name_and_type_index
                  - 1]
             .info)
            ->name_index);
}

char *
get_method_descriptor(class_t *class_struct, size_t position)
{

    return get_utf8(
        class_struct,
        ((name_and_type_info_t *)class_struct
             ->constant_pool
                 [((ref_info_t *)class_struct->constant_pool[position - 1].info)
                      ->name_and_type_index
                  - 1]
             .info)
            ->descriptor_index);
}

char *
get_method_class(class_t *class_struct, size_t position)
{
    return get_class_name(
        class_struct,
        ((ref_info_t *)class_struct->constant_pool[position - 1].info)
            ->class_index);
}

char *
attribute_name_lookup(attribute_type_t attribute_type)
{
    char *attribute_type_str;

    switch (attribute_type) {
        case ATTR_UNPARSED: attribute_type_str = "Unparsed"; break;
        case ATTR_CODE: attribute_type_str = "Code"; break;
        default: attribute_type_str = NULL;
    }

    return attribute_type_str;
}

char *
get_utf8(class_t *class_struct, uint32_t position)
{
    utf8_info_t *utf_info;

    utf_info = (class_struct->constant_pool[position - 1]).info;

    return (char *)utf_info->bytes;
}

method_info_t *
find_method_special(class_t *class_struct, char *method_name,
                    char *method_descriptor)
{
    method_info_t *methods, *method;

    methods = class_struct->methods;
    method = NULL;

    for (size_t i = 0; i < class_struct->methods_count; i++) {
        if (strcmp(method_name, methods[i].name) == 0
            && strcmp(method_descriptor, methods[i].descriptor) == 0) {
            method = methods + i;
            break;
        }
    }

    return method;
}

method_info_t *
find_method_virtual(class_t *class_struct, char *method_name,
                    char *method_descriptor)
{
    method_info_t *method;
    method = NULL;

    while (true) {
        method = find_method_special(class_struct, method_name,
                                     method_descriptor);
        if (method != NULL || class_struct->super_class == NULL) {
            break;
        }
    }

    return method;
}

attribute_code_t *
find_code(method_info_t *method)
{
    attribute_code_t *code;

    for (size_t i = 0; i < method->attributes_count; i++) {
        if (method->attributes[i].tag == ATTR_CODE) {
            code = method->attributes[i].info;
        }
    }

    return code;
}

bool
is_subclass(vm_context_t *context, char *super_class_name, char *subclass_name)
{
    class_t *subclass;

    subclass = resolve_class(context, subclass_name);

    if (subclass == NULL) {
        return false;
    }

    while (1) {
        if (subclass->this_class == super_class_name) {
            return true;
        } else {
            if (subclass->super_class != NULL) {
                subclass = resolve_class(context, subclass->super_class);
            } else {
                return false;
            }
        }
    }
}

size_t
count_fields(vm_context_t *context, class_t *class_struct, flags_t t_acc_flags,
             flags_t f_acc_flags)
{
    size_t count;

    count = 0;
    while (class_struct != NULL) {
        for (size_t i = 0; i < class_struct->fields_count; i++) {
            if (check_flags(class_struct->fields[i].access_flags, t_acc_flags,
                            f_acc_flags)) {
                count++;
            }
        }

        if (class_struct->super_class != NULL) {
            class_struct = resolve_class(context, class_struct->super_class);
        } else {
            class_struct = NULL;
        }
    }

    return count;
}

#include <limits.h>
#include <linux/limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "boot_loader.h"
#include "class.h"
#include "jvm.h"

u4_t
read_u4(FILE *class_file)
{
    u4_t num;

    fread(&num, sizeof(u4_t), 1, class_file);

    return ((num >> 24) & 0xff) | ((num << 8) & 0xff0000)
           | ((num >> 8) & 0xff00) | ((num << 24) & 0xff000000);
}

u2_t
read_u2(FILE *class_file)
{
    u2_t num;

    fread(&num, sizeof(u2_t), 1, class_file);

    return (num >> 8) | (num << 8);
}

u1_t
read_u1(FILE *class_file)
{
    u1_t num;

    fread(&num, sizeof(u1_t), 1, class_file);

    return num;
}

u1_t *
read_bytes(FILE *class_file, size_t length)
{
    u1_t *dest;

    dest = malloc(sizeof(u1_t) * length);

    fread(dest, sizeof(u1_t), length, class_file);

    return dest;
}

u1_t *
read_string_bytes(FILE *class_file, size_t length)
{
    u1_t *dest;

    dest = calloc(length + 1, sizeof(u1_t));

    fread(dest, sizeof(u1_t), length, class_file);

    return dest;
}

ref_info_t *
read_ref_info(FILE *class_file)
{
    ref_info_t *ref;

    ref = malloc(sizeof(ref_info_t));

    ref->class_index = read_u2(class_file);
    ref->name_and_type_index = read_u2(class_file);

    return ref;
}

name_and_type_info_t *
read_name_and_type_info(FILE *class_file)
{
    name_and_type_info_t *ref;

    ref = malloc(sizeof(name_and_type_info_t));

    ref->name_index = read_u2(class_file);
    ref->descriptor_index = read_u2(class_file);

    return ref;
}

utf8_info_t *
read_utf8(FILE *class_file)
{
    utf8_info_t *ref;

    ref = malloc(sizeof(utf8_info_t));

    ref->length = read_u2(class_file);
    ref->bytes = read_string_bytes(class_file, ref->length);

    return ref;
}

string_info_t *
read_string(FILE *class_file)
{
    string_info_t *ref;

    ref = malloc(sizeof(string_info_t));

    ref->string_index = read_u2(class_file);

    return ref;
}

uint32_t
read_cp_info(FILE *class_file, cp_info_t *dest)
{
    constant_type_t tag;

    tag = (constant_type_t)read_u1(class_file);
    dest->tag = tag;

    switch (tag) {
        case CONSTANT_FIELD_REF:
        case CONSTANT_METHOD_REF:
        case CONSTANT_INTERFACE_METHOD_REF:
            dest->info = read_ref_info(class_file);
            break;
        case CONSTANT_CLASS:
            dest->info = malloc(sizeof(class_info_t));
            ((class_info_t *)dest->info)->name_index = read_u2(class_file);
            break;
        case CONSTANT_NAME_AND_TYPE:
            dest->info = read_name_and_type_info(class_file);
            break;
        case CONSTANT_UTF8: dest->info = read_utf8(class_file); break;
        case CONSTANT_STRING: dest->info = read_string(class_file); break;
        default:
            fprintf(stderr, "Error: Unknown constant type %d\n", tag);
            exit(1);
            return 0;
    }
    return 1;
}

bool
read_unparsed_attribute(FILE *class_file, u2_t attribute_name_index,
                        attribute_info_t *dest)
{
    attribute_unparsed_t *unparsed_info;
    u4_t                  attribute_length;

    unparsed_info = malloc(sizeof(attribute_unparsed_t));
    attribute_length = read_u4(class_file);

    unparsed_info->attribute_name_index = attribute_name_index;
    unparsed_info->attribute_length = attribute_length;
    unparsed_info->info = read_bytes(class_file, attribute_length);

    dest->tag = ATTR_UNPARSED;
    dest->info = unparsed_info;

    return true;
}

bool
read_exception_table_entry(FILE                    *class_file,
                           exception_table_entry_t *exception_table_entry)
{
    exception_table_entry->start_pc = read_u2(class_file);
    exception_table_entry->end_pc = read_u2(class_file);
    exception_table_entry->handler_pc = read_u2(class_file);
    exception_table_entry->catch_type = read_u2(class_file);

    return true;
}

/* read_code_attribute uses this yet this is internal function, I don't want
   put it into header */
bool read_attribute(FILE *class_file, class_struct_t *class_struct,
                    attribute_info_t *dest);

bool
read_code_attribute(FILE *class_file, class_struct_t *class_struct,
                    attribute_info_t *dest)
{
    attribute_code_t *code_info = malloc(sizeof(attribute_code_t));
    u4_t              attribute_length = read_u4(class_file);

    code_info->max_stack = read_u2(class_file);
    code_info->max_locals = read_u2(class_file);
    code_info->code_length = read_u4(class_file);
    code_info->code = read_bytes(class_file, code_info->code_length);
    code_info->exception_table_length = read_u2(class_file);
    code_info->exception_table = malloc(sizeof(exception_table_entry_t)
                                        * code_info->exception_table_length);
    for (size_t i = 0; i < code_info->exception_table_length; i++) {
        read_exception_table_entry(class_file, code_info->exception_table + i);
    }
    code_info->attributes_count = read_u2(class_file);
    code_info->attributes = malloc(sizeof(attribute_info_t)
                                   * code_info->attributes_count);
    for (size_t i = 0; i < code_info->attributes_count; i++) {
        read_attribute(class_file, class_struct, code_info->attributes + i);
    }

    dest->tag = ATTR_CODE;
    dest->info = code_info;

    return true;
}

bool
read_attribute(FILE *class_file, class_struct_t *class_struct,
               attribute_info_t *dest)
{
    u2_t  attribute_name_index;
    char *attribute_name;

    attribute_name_index = read_u2(class_file);
    attribute_name = get_utf8(class_struct, attribute_name_index);

    if (!strcmp(attribute_name, attribute_name_lookup(ATTR_CODE))) {
        read_code_attribute(class_file, class_struct, dest);
    } else {
        read_unparsed_attribute(class_file, attribute_name_index, dest);
    }

    return true;
}

bool
read_field_info(FILE *class_file, class_struct_t *class_struct,
                field_info_t *dest)
{
    dest->access_flags = read_u2(class_file);
    dest->name = get_utf8(class_struct, read_u2(class_file));
    dest->descriptor = get_utf8(class_struct, read_u2(class_file));
    dest->attributes_count = read_u2(class_file);
    dest->attributes = malloc(sizeof(attribute_info_t)
                              * dest->attributes_count);
    for (size_t i = 0; i < dest->attributes_count; i++) {
        read_attribute(class_file, class_struct, dest->attributes + i);
    }
    return true;
}

bool
read_method(FILE *class_file, class_struct_t *class_struct, method_info_t *dest)
{
    dest->access_flags = read_u2(class_file);
    dest->name = get_utf8(class_struct, read_u2(class_file));
    dest->descriptor = get_utf8(class_struct, read_u2(class_file));
    dest->attributes_count = read_u2(class_file);
    dest->attributes = malloc(sizeof(attribute_info_t)
                              * dest->attributes_count);
    for (size_t i = 0; i < dest->attributes_count; i++) {
        read_attribute(class_file, class_struct, dest->attributes + i);
    }
    return true;
}

bool
read_class(FILE *class_file, class_struct_t *dest)
{
    uint32_t magic;

    magic = read_u4(class_file);

    if (magic != MAGIC) {
        fprintf(stderr, "ERROR: wrong type of file\n");
        return false;
    }

    dest->minor = read_u2(class_file);
    dest->major = read_u2(class_file);

    dest->constant_pool_count = read_u2(class_file);
    dest->constant_pool = malloc(sizeof(cp_info_t)
                                 * (dest->constant_pool_count - 1));
    for (size_t i = 0; i < dest->constant_pool_count - 1; i++) {
        uint32_t return_code = read_cp_info(class_file,
                                            dest->constant_pool + i);
        if (!return_code) {
            break;
        }
    }

    dest->access_flags = read_u2(class_file);

    dest->this_class = get_class_name(dest, read_u2(class_file));
    dest->super_class = get_class_name(dest, read_u2(class_file));

    dest->interfaces_count = read_u2(class_file);
    dest->interfaces = malloc(sizeof(u2_t) * dest->interfaces_count);
    for (size_t i = 0; i < dest->interfaces_count; i++) {
        dest->interfaces[i] = get_class_name(dest, read_u2(class_file));
    }

    dest->fields_count = read_u2(class_file);
    dest->fields = malloc(sizeof(field_info_t) * dest->fields_count);
    for (size_t i = 0; i < dest->fields_count; i++) {
        read_field_info(class_file, dest, dest->fields + i);
    }

    dest->methods_count = read_u2(class_file);
    dest->methods = malloc(sizeof(method_info_t) * dest->methods_count);
    for (size_t i = 0; i < dest->methods_count; i++) {
        read_method(class_file, dest, dest->methods + i);
    }

    dest->attributes_count = read_u2(class_file);
    dest->attributes = malloc(sizeof(attribute_info_t *)
                              * dest->attributes_count);
    for (size_t i = 0; i < dest->attributes_count; i++) {
        read_attribute(class_file, dest, dest->attributes + i);
    }

    return true;
}

class_struct_t *
load_class(vm_context_t *context, char *class_name)
{
    FILE           *class_file;
    char           *path;
    class_struct_t *new_class;
    bool            success;

    if (context->class_table->count == context->class_table->size) {
        size_t new_size;

        if (context->class_table->size == 0) {
            new_size = 10;
        } else {
            new_size = context->class_table->size * 2;
        }

        context->class_table->size = new_size;
        context->class_table->class_pool = realloc(
            context->class_table->class_pool,
            sizeof(class_struct_t *) * new_size);
    }
    success = false;
    path = malloc(sizeof(char) * PATH_MAX);

    for (size_t i = 0; i < context->classpath_size; i++) {
        format_class_path(path, context->classpath[i], class_name);
        class_file = fopen(path, "rb");

        if (class_file) {

            new_class = malloc(sizeof(class_struct_t));
            context->class_table->class_pool[context->class_table->count] =
                new_class;
            read_class(class_file, new_class);
            context->class_table->count++;

            if (new_class->super_class != NULL
                && find_class(context, new_class->super_class) == NULL) {
                success = load_class(context, new_class->super_class);
            } else {
                success = true;
            }

            fclose(class_file);
            break;
        }
    }

    if (success) {
        printf("Class %s loaded.\n", class_name);
    } else {
        fprintf(stderr, "Error while loading %s class\n", class_name);
    }

    free(path);
    return new_class;
}

class_struct_t *
resolve_class(vm_context_t *context, char *class_name)
{
    class_struct_t *resolved_class;

    resolved_class = find_class(context, class_name);

    if (resolved_class == NULL) {
        resolved_class = load_class(context, class_name);
    }

    return resolved_class;
}


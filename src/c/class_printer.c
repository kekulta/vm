#include <stdio.h>
#include "array.h"
#include "class.h"
#include "class_printer.h"
#include "instance.h"
#include "jvm.h"
#include "object.h"

void
print_attribute(class_t *class_struct, attribute_info_t *attribute_info,
                char *prefix)
{
    attribute_type_t attribute_type = attribute_info->tag;
    char            *attribute_type_str = attribute_name_lookup(attribute_type);

    printf("%s%s\n", prefix, attribute_type_str);
}

void
print_class(class_t *class_struct)
{
    printf("CLASS_STRUCT:\n"
           "\tminor: %d\n"
           "\tmajor: %d\n"
           "\tconstant_pool_count: %zu\n"
           "\tconstant_pool:\n",
           class_struct->minor, class_struct->major,
           class_struct->constant_pool_count);
    for (size_t i = 0; i < class_struct->constant_pool_count - 1; i++) {
        printf("\t#%zu, ", i + 1);
        if (class_struct->constant_pool[i].tag == CONSTANT_UTF8) {
            printf("type: %d"
                   "\t// %s\n",
                   class_struct->constant_pool[i].tag,
                   ((utf8_info_t *)class_struct->constant_pool[i].info)->bytes);
        } else {
            printf("type: %d\n", class_struct->constant_pool[i].tag);
        }
    }

    printf("access_flags: %b\n", class_struct->access_flags);
    if (class_struct->access_flags & ACC_CLASS_PUBLIC) {
        printf("\tACC_CLASS_PUBLIC\n");
    }
    if (class_struct->access_flags & ACC_CLASS_FINAL) {
        printf("\tACC_CLASS_FINAL\n");
    }
    if (class_struct->access_flags & ACC_CLASS_SUPER) {
        printf("\tACC_CLASS_SUPER\n");
    }
    if (class_struct->access_flags & ACC_CLASS_INTERFACE) {
        printf("\tACC_CLASS_INTERFACE\n");
    }
    if (class_struct->access_flags & ACC_CLASS_ABSTRACT) {
        printf("\tACC_CLASS_ABSTRACT\n");
    }
    if (class_struct->access_flags & ACC_CLASS_SYNTHETIC) {
        printf("\tACC_CLASS_SYNTHETIC\n");
    }
    if (class_struct->access_flags & ACC_CLASS_ANNOTATION) {
        printf("\tACC_CLASS_ANNOTATION\n");
    }
    if (class_struct->access_flags & ACC_CLASS_ENUM) {
        printf("\tACC_CLASS_ENUM\n");
    }

    printf("this_class: %s\n", class_struct->this_class);
    printf("super_class: %s\n", class_struct->super_class);

    printf("interfaces_count: %zu\n", class_struct->interfaces_count);
    // TODO: print interfaces

    printf("fields_count: %zu\n", class_struct->fields_count);
    printf("fields:\n");
    for (size_t i = 0; i < class_struct->fields_count; i++) {
        printf("\t%s %s\n", class_struct->fields[i].name,
               class_struct->fields[i].descriptor);

        printf("\taccess_flags: %b\n", class_struct->fields[i].access_flags);
        if (class_struct->fields[i].access_flags & ACC_FIELD_PUBLIC) {
            printf("\t\tACC_FIELD_PUBLIC\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_PRIVATE) {
            printf("\t\tACC_FIELD_PRIVATE\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_PROTECTED) {
            printf("\t\tACC_FIELD_PROTECTED\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_STATIC) {
            printf("\t\tACC_FIELD_STATIC\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_FINAL) {
            printf("\t\tACC_FIELD_FINAL\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_VOLATILE) {
            printf("\t\tACC_FIELD_VOLATILE\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_TRANSIENT) {
            printf("\t\tACC_FIELD_TRANSIENT\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_SYNTHETIC) {
            printf("\t\tACC_FIELD_SYNTHETIC\n");
        }
        if (class_struct->fields[i].access_flags & ACC_FIELD_ENUM) {
            printf("\t\tACC_FIELD_ENUM\n");
        }
    }

    printf("methods_count: %zu\n", class_struct->methods_count);
    printf("methods:\n");
    for (size_t i = 0; i < class_struct->methods_count; i++) {
        printf("\t%s %s\n", class_struct->methods[i].descriptor,
               class_struct->methods[i].name);
        for (size_t j = 0; j < class_struct->methods[i].attributes_count; j++) {
            print_attribute(class_struct,
                            class_struct->methods[i].attributes + j, "\t\t");
        }
    }

    printf("attributes_count: %zu\n", class_struct->attributes_count);
    for (size_t i = 0; i < class_struct->attributes_count; i++) {
        print_attribute(class_struct, class_struct->attributes + i, "\t");
    }

    return;
}

void
print_object(object_t *object_struct)
{
    field_t     field;
    instance_t *instance;
    bool        is_aarray;
    size_t      array_size;

    printf("%p:\n", object_struct);
    if (object_struct->descriptor[0] != '[') {
        instance = object_struct->value.instance;
        printf("class_name: %s\n", instance->class_name);
        printf("fields_count: %zu\n", instance->fields_count);
        for (size_t i = 0; i < instance->fields_count; i++) {
            field = instance->fields[i];
            printf("\t%s %s %s\n", field.field_name, field.descriptor,
                   field.field_name);
            switch (field.descriptor[0]) {
                case 'I': printf("\t\tINT: %d\n", field.value.jint); break;
                case 'L':
                case '[':
                    printf("\t\tREFERENCE: %p\n", field.value.object);
                    break;
                default: printf("\t\tUNKNOWN");
            }
        }
    } else {
        printf("ARR:\n");
        printf("\ttype: %s\n", object_struct->descriptor);
        printf("\tvalues: {");
        is_aarray = object_struct->descriptor[1] != 'I';

        if (is_aarray) {
            array_size = object_struct->value.aarray->size;
        } else {
            array_size = object_struct->value.iarray->size;
        }

        for (size_t i = 0; i < array_size; i++) {
            if (is_aarray) {
                printf("%p", object_struct->value.aarray->value[i]);
            } else {
                printf("%d", object_struct->value.iarray->value[i]);
            }
            if (i < array_size) {
                printf(", ");
            }
        }
        printf("}\n");
    }
}

void
print_object_table(object_table_t *object_table)
{
    printf("Object table:\n\n");
    printf("count: %zu\n", object_table->count);
    for (int i = 0; i < object_table->count; i++) {
        print_object(object_table->object_pool[i]);
        printf("\n");
    }
}

void
print_class_table(class_table_t *class_table)
{
    printf("size: %zu\ncount: %zu\n\n", class_table->size, class_table->count);

    for (size_t i = 0; i < class_table->count; i++) {
        print_class(class_table->class_pool[i]);
        printf("\n\n");
    }
}

void
print_value(vm_value_t value)
{
    switch (value.descriptor[0]) {
        case 'I': printf("INT: %d\n", value.content.jint); break;
        case 'L':
        case '[':
            if (value.content.object == NULL) {
                printf("OBJECT: %p\n", value.content.object);
            } else {
                print_object(value.content.object);
            }
            break;
    }
}

void
print_frame(vm_context_t *context, frame_t *frame)
{
    printf("FRAME:\n");
    printf("descriptor: %s\n", frame->descriptor);
    printf("max_locals: %zu\n", frame->max_locals);
    printf("locals:\n");
    for (size_t i = 0; i < frame->max_locals; i++) {
        print_value(frame->locals[i]);
    }
    printf("stack:\n");
    for (size_t i = 0; i < frame->stack_count; i++) {
        print_value(frame->stack[i]);
    }
}

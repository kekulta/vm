#ifndef CLASS_PRINTER_H
#define CLASS_PRINTER_H

#include "class.h"
#include "jvm.h"

void print_class(class_t *class_struct);
void print_class_table(class_table_t *class_table);

void print_object(object_t *object_struct);
void print_object_table(object_table_t *object_table);

void print_frame(vm_context_t *context, frame_t *frame);
void print_value(vm_value_t value);
#endif

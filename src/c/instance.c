#include <stdbool.h>
#include "class.h"
#include "instance.h"
#include "jvm.h"

bool
is_instance(vm_context_t *context, class_struct_t *super_class,
            instance_struct_t *instance)
{
    return is_subclass(context, super_class, instance->class_name);
}

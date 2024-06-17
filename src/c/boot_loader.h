#ifndef BOOT_LOADER_H
#define BOOT_LOADER_H

#include <stdbool.h>
#include "class.h"
#include "jvm.h"

class_struct_t  *resolve_class(vm_context_t *context, char *class_name);

#endif

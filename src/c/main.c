#include <limits.h>
#include <linux/limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "boot_loader.h"
#include "class.h"
#include "class_printer.h"
#include "cli.h"
#include "interpreter.h"

static char DEFAULT_BOOTPATH[] = "std";
static char DEFAULT_CLASSPATH[] = ".";

frame_t *
create_entry_frame(vm_context_t *context, class_t *class_struct)
{
    flags_t main_flags, method_flags;
    char   *main_name, *main_descriptor, *method_name, *method_descriptor;
    method_info_t    *method;
    frame_t          *main_frame;
    attribute_code_t *code;

    main_name = "main";
    main_descriptor = "([Ljava/lang/String;)V";
    main_flags = ACC_METHOD_PUBLIC & ACC_METHOD_STATIC;

    method = find_method_virtual(class_struct, main_name, main_descriptor);

    if (method == NULL
        || !check_flags(method->access_flags, main_flags, NO_FLAGS)) {
        fprintf(stderr, "Can't find main method.");
        return NULL;
    }

    vm_value_t *args = malloc(sizeof(vm_value_t) * 1);
    size_t      sizes[] = {2, 3, 4};

    args[0].descriptor = "[Ljava/lang/String;";
    printf("args[0]: %s\n", args[0].descriptor);
    args[0].content.object = allocate_array_a(context, "[Ljava/lang/String;", 1,
                                              sizes);


    main_frame = construct_frame(context, class_struct, method, args);

    return main_frame;
}

int
main(int argc, char **argv)
{
    char         *class_name, *std_lib;
    class_t      *entry_point;
    uint32_t      magic;
    frame_t      *frame;
    vm_context_t *context;

    context = calloc(1, sizeof(vm_context_t));
    context->class_table = calloc(1, sizeof(class_table_t));
    context->object_table = calloc(1, sizeof(object_table_t));
    context->frame_stack = calloc(1, sizeof(frame_stack_t));

    if (argc < 2) {
        fprintf(stderr, "ERROR: class name must be provided\n");
        exit(1);
    }

    std_lib = extract_std_lib_path(argv[0], DEFAULT_BOOTPATH);
    context->classpath_size = extract_classpathes(
        argc, argv, std_lib, DEFAULT_CLASSPATH, &context->classpath);

    class_name = malloc(sizeof(char) * (strlen(argv[argc - 1]) + 1));
    format_class_name(class_name, argv[argc - 1]);

    entry_point = resolve_class(context, class_name);

    if (entry_point == NULL) {
        fprintf(stderr, "ERROR: couldn't find or load class %s\n", class_name);
        exit(1);
    }

    print_class_table(context->class_table);
    print_object_table(context->object_table);

    frame = create_entry_frame(context, entry_point);
    if (frame == NULL) {
        printf("Error while creating entry frame\n");
        exit(1);
    } else {
        print_frame(context, frame);
        push_frame(context, frame);
    }

    if (start_interpreting(context)) {
        printf("Program executed\n");
    }

    return 0;
}

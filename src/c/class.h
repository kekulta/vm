#ifndef CLASS_H
#define CLASS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include "types.h"

#define MAGIC    0xCAFEBABE
#define NO_FLAGS 0

typedef struct vm_context    vm_context_t;
typedef struct object_struct object_t;

typedef uint8_t              u1_t;
typedef uint16_t             u2_t;
typedef uint32_t             u4_t;

typedef enum {
    CONSTANT_CLASS = 7,
    CONSTANT_FIELD_REF = 9,
    CONSTANT_METHOD_REF = 10,
    CONSTANT_INTERFACE_METHOD_REF = 11,
    CONSTANT_STRING = 8,
    CONSTANT_INTEGER = 3,
    CONSTANT_FLOAT = 4,
    CONSTANT_LONG = 5,
    CONSTANT_DOUBLE = 6,
    CONSTANT_NAME_AND_TYPE = 12,
    CONSTANT_UTF8 = 1,
    CONSTANT_METHOD_HANDLE = 15,
    CONSTANT_METHOD_TYPE = 16,
    CONSTANT_INVOKE_DYNAMIC = 18,
} constant_type_t;

typedef enum {
    ATTR_UNPARSED,
    ATTR_CODE,
} attribute_type_t;

typedef enum {
    ACC_CLASS_PUBLIC = 0x0001,
    ACC_CLASS_FINAL = 0x0010,
    ACC_CLASS_SUPER = 0x0020,
    ACC_CLASS_INTERFACE = 0x0200,
    ACC_CLASS_ABSTRACT = 0x0400,
    ACC_CLASS_SYNTHETIC = 0x1000,
    ACC_CLASS_ANNOTATION = 0x2000,
    ACC_CLASS_ENUM = 0x4000,
} acc_class_flag_t;

typedef enum {
    ACC_FIELD_PUBLIC = 0x0001,
    ACC_FIELD_PRIVATE = 0x0002,
    ACC_FIELD_PROTECTED = 0x0004,
    ACC_FIELD_STATIC = 0x0008,
    ACC_FIELD_FINAL = 0x0010,
    ACC_FIELD_VOLATILE = 0x0040,
    ACC_FIELD_TRANSIENT = 0x0080,
    ACC_FIELD_SYNTHETIC = 0x1000,
    ACC_FIELD_ENUM = 0x4000,
} acc_field_flag_t;

typedef enum {
    ACC_METHOD_PUBLIC = 0x0001,
    ACC_METHOD_PRIVATE = 0x0002,
    ACC_METHOD_PROTECTED = 0x0004,
    ACC_METHOD_STATIC = 0x0008,
    ACC_METHOD_FINAL = 0x0010,
    ACC_METHOD_SYNCHRONIZED = 0x0020,
    ACC_METHOD_BRIDGE = 0x0040,
    ACC_METHOD_VARARGS = 0x0080,
    ACC_METHOD_NATIVE = 0x0100,
    ACC_METHOD_ABSTRACT = 0x0400,
    ACC_METHOD_STRICT = 0x0800,
    ACC_METHOD_SYNTHETIC = 0x1000,
} acc_method_flag_t;

typedef struct cp_info {
    constant_type_t tag;
    /* Concrete type depends on the tag value */
    void           *info;
} cp_info_t;

typedef struct ref_info {
    u2_t class_index;
    u2_t name_and_type_index;
} ref_info_t;

typedef struct class_info {
    u2_t name_index;
} class_info_t;

typedef struct name_and_type_info {
    u2_t name_index;
    u2_t descriptor_index;
} name_and_type_info_t;

typedef struct utf8_info {
    /* Number of meaningful characters */
    u2_t  length;
    /* Actually contains length+1 bytes of data, the last byte is the \0 byte */
    /* Correct C string */
    u1_t *bytes;
} utf8_info_t;

typedef struct string_info {
    u2_t string_index;
} string_info_t;

typedef struct attribute_info {
    attribute_type_t tag;
    /* Concrete type depends on the tag value */
    void            *info;
} attribute_info_t;

typedef struct attribute_unparsed {
    u2_t  attribute_name_index;
    u4_t  attribute_length;
    u1_t *info;
} attribute_unparsed_t;

typedef struct exception_table_entry {
    u2_t start_pc;
    u2_t end_pc;
    u2_t handler_pc;
    u2_t catch_type;
} exception_table_entry_t;

typedef struct attribute_code {
    u2_t                     max_stack;
    u2_t                     max_locals;
    u4_t                     code_length;
    u1_t                    *code;
    u2_t                     exception_table_length;
    exception_table_entry_t *exception_table;
    u2_t                     attributes_count;
    attribute_info_t        *attributes;
} attribute_code_t;

typedef struct method_info {
    flags_t           access_flags;
    char             *name;
    descriptor_t      descriptor;
    size_t            attributes_count;
    attribute_info_t *attributes;
} method_info_t;

typedef struct field_info {
    flags_t           access_flags;
    char             *name;
    char             *descriptor;
    size_t            attributes_count;
    attribute_info_t *attributes;
} field_info_t;

typedef struct class_struct {
    uint16_t          minor;
    uint16_t          major;
    size_t            constant_pool_count;
    cp_info_t        *constant_pool;
    flags_t           access_flags;
    char             *this_class;
    descriptor_t      descriptor;
    char             *super_class;
    size_t            interfaces_count;
    char            **interfaces;
    size_t            fields_count;
    field_info_t     *fields;
    size_t            methods_count;
    method_info_t    *methods;
    size_t            attributes_count;
    attribute_info_t *attributes;
    object_t         *static_instance;
} class_t;

method_info_t    *find_method_special(class_t *class_struct, char *method_name,
                                      char *method_descriptor);

method_info_t    *find_method_virtual(class_t *class_struct, char *method_name,
                                      char *method_descriptor);

char             *attribute_name_lookup(attribute_type_t attribute_type);

char             *get_utf8(class_t *class_struct, uint32_t position);

char             *get_class_name(class_t *class_struct, size_t position);

char             *get_string(class_t *class_struct, size_t position);

char             *get_method_name(class_t *class_struct, size_t position);

char             *get_method_descriptor(class_t *class_struct, size_t position);

char             *get_method_class(class_t *class_struct, size_t position);

size_t            count_fields(vm_context_t *context, class_t *class_struct,
                               flags_t t_acc_flags, flags_t f_acc_flags);

attribute_code_t *find_code(method_info_t *method);

bool              is_subclass(vm_context_t *context, char *super_class_name,
                              char *subclass_name);

#endif

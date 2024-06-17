typedef struct vm_context vm_context_t;

typedef enum {
    op_ldc = 0x12,
    op_invokestatic = 0xb8,
    op_return = 0xb1,
    op_aload_0 = 0x2a,
} op_code_t;

bool start_interpreting(vm_context_t *context);

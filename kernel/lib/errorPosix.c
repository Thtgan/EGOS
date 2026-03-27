#include<lib/errorPosix.h>
#include<kit/atomic.h>
#include<debug.h>
#include<print.h>
#include<multitask/schedule.h>

ErrorContext error_global_context = {
    .hasError = false,
    .depth = 0,
    .padding = {0},
    .stackTop = NULL,
    .errorStack = NULL,
    .deferredError = ERROR_OK,
};

static ErrorFrame boot_error_frames[CONFIG_ERROR_STACK_SIZE];

ErrorContext error_boot_context = {
    .hasError = false,
    .depth = 0,
    .padding = {0},
    .stackTop = boot_error_frames,
    .errorStack = boot_error_frames,
    .deferredError = ERROR_OK,
};

IsrErrorContext isr_error_context = {
    .pendingError = ERROR_OK,
    .lastIRQnumber = 0,
};

void error_posix_init() {
#ifdef CONFIG_ENABLE_POSIX_ERROR
    // Initialize boot context with its dedicated error stack buffer
    error_ctx_init(&error_boot_context, boot_error_frames);
    
    // Initialize global context (no dedicated error stack buffer)
    error_ctx_init(&error_global_context, NULL);
#endif
    
    ATOMIC_STORE(&isr_error_context.pendingError, ERROR_OK);
    ATOMIC_STORE(&isr_error_context.lastIRQnumber, 0);
}

void error_ctx_init(ErrorContext* ctx, ErrorFrame* error_stack) {
#ifdef CONFIG_ENABLE_POSIX_ERROR
    DEBUG_ASSERT_SILENT(ctx != NULL);
    // Note: error_stack can be NULL for contexts like error_global_context
    // that don't have a dedicated error stack buffer
    
    ctx->hasError = false;
    ctx->depth = 0;
    ctx->stackTop = error_stack;
    ctx->errorStack = error_stack;
    ctx->deferredError = ERROR_OK;
#endif
}

ErrorContext* error_ctx_current() {
#ifdef CONFIG_ENABLE_POSIX_ERROR
    Thread* current = schedule_getCurrentThread();
    if (current != NULL) {
        return &current->error_ctx;
    }
#endif
    return &error_boot_context;
}

void error_print_stack() {
    ErrorContext* ctx = error_ctx_current();
    
    if (!ctx->hasError || ctx->depth == 0) {
        debug_printf("[ERROR STACK] No errors pending\n");
        return;
    }
    
    debug_printf("[ERROR STACK] Depth: %u, Errors:\n", ctx->depth);
    
    for (Uint32 i = 0; i < ctx->depth; i++) {
        ErrorFrame* frame = &ctx->errorStack[i];
        ErrorCode code = frame->code;
        
        debug_printf("  [%u] Code: 0x%08X\n", i, code);
        debug_printf("       Domain: 0x%02X, Class: 0x%02X, Severity: 0x%X\n",
                     ERROR_GET_DOMAIN(code),
                     ERROR_GET_CLASS(code),
                     ERROR_GET_SEVERITY(code));
        
#if CONFIG_ERROR_FRAME_DEBUG == 1
        if (frame->file != NULL && frame->line > 0) {
            debug_printf("       Location: %s:%u\n", frame->file, frame->line);
        }
#endif
    }
}

void error_to_string(ErrorCode code, char* buffer, Uint32 buffer_size) {
    if (buffer_size < 32) {
        return;
    }
    
    print_snprintf(buffer, buffer_size,
                   "0x%02X.0x%02X.0x%X.0x%02X.%u",
                   ERROR_GET_DOMAIN(code),
                   ERROR_GET_CLASS(code),
                   ERROR_GET_SEVERITY(code),
                   ERROR_GET_SUBDOMAIN(code),
                   ERROR_GET_CODE(code));
}

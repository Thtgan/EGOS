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

void error_posix_init(void) {
    error_boot_context.hasError = false;
    error_boot_context.depth = 0;
    error_boot_context.stackTop = boot_error_frames;
    error_boot_context.errorStack = boot_error_frames;
    error_boot_context.deferredError = ERROR_OK;
    
    error_global_context.hasError = false;
    error_global_context.depth = 0;
    error_global_context.stackTop = NULL;
    error_global_context.errorStack = NULL;
    error_global_context.deferredError = ERROR_OK;
    
    ATOMIC_STORE(&isr_error_context.pendingError, ERROR_OK);
    ATOMIC_STORE(&isr_error_context.lastIRQnumber, 0);
}

ErrorContext* error_ctx_current(void) {
#ifdef CONFIG_ENABLE_POSIX_ERROR
    Thread* current = schedule_getCurrentThread();
    if (current != NULL) {
        return &current->error_ctx;
    }
#endif
    return &error_boot_context;
}

void error_print_stack(void) {
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

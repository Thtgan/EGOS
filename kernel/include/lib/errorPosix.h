#ifndef __LIB_ERROR_POSIX_H
#define __LIB_ERROR_POSIX_H

#include<debug.h>
#include<kit/atomic.h>
#include<kit/config.h>
#include<kit/types.h>
#include<lib/errorCode.h>
#include<memory/memory.h>

// 前向声明以打破循环引用
typedef struct Thread Thread;
extern Thread* schedule_getCurrentThread(void);

#ifndef CONFIG_ERROR_STACK_SIZE
#define CONFIG_ERROR_STACK_SIZE     4
#endif

#if defined(CONFIG_ERROR_FRAME_DEBUG)

typedef struct ErrorFrame {
    ErrorCode code;
    ConstCstring file;
    Uint32 line;
} __attribute__((packed)) ErrorFrame;

#else

typedef struct ErrorFrame {
    ErrorCode code;
} ErrorFrame;

#endif

typedef struct ErrorContext {
    bool hasError;
    Uint8 depth;
    Uint8 padding[2];
    
    ErrorFrame* stackTop;
    ErrorFrame* errorStack;
    
    ErrorCode deferredError;
} __attribute__((packed)) ErrorContext;

typedef struct IsrErrorContext {
    volatile ErrorCode pendingError;
    volatile Uint16 lastIRQnumber;
} __attribute__((packed)) IsrErrorContext;

extern ErrorContext error_global_context;   // 全局上下文（ISR 使用）
extern ErrorContext error_boot_context;     // 引导阶段上下文
extern IsrErrorContext isr_error_context;   // ISR 专用上下文

ErrorContext* error_ctx_current(void);

#if defined(CONFIG_ERROR_FRAME_DEBUG)
static inline void error_push(ErrorCode code, ConstCstring message,
                              ConstCstring file, Uint32 line) {
    ErrorContext* ctx = error_ctx_current();
    
    if (ctx->depth >= CONFIG_ERROR_STACK_SIZE) {
        memory_memmove(ctx->errorStack, ctx->errorStack + 1, sizeof(ErrorFrame) * (CONFIG_ERROR_STACK_SIZE - 1));
        ctx->depth = CONFIG_ERROR_STACK_SIZE - 1;
    }
    
    ErrorFrame* frame = &ctx->errorStack[ctx->depth++];
    frame->code = code;
    frame->file = file ? file : "";
    frame->line = line;
    
    ctx->hasError = true;
}

#else

static inline void error_push(ErrorCode code) {
    ErrorContext* ctx = error_ctx_current();
    
    if (ctx->depth >= CONFIG_ERROR_STACK_SIZE) {
        memory_memmove(ctx->errorStack, ctx->errorStack + 1, sizeof(ErrorFrame) * (CONFIG_ERROR_STACK_SIZE - 1));
        ctx->depth = CONFIG_ERROR_STACK_SIZE - 1;
    }
    
    ErrorFrame* frame = &ctx->errorStack[ctx->depth++];
    frame->code = code;
    
    ctx->hasError = true;
}

#endif

static inline bool error_pending(void) {
    return error_ctx_current()->hasError;
}

static inline ErrorCode error_get_code(void) {
    ErrorContext* ctx = error_ctx_current();
    if (!ctx->hasError || ctx->depth == 0) {
        return ERROR_OK;
    }
    return ctx->errorStack[ctx->depth - 1].code;
}

static inline ErrorFrame* error_get_frame(void) {
    ErrorContext* ctx = error_ctx_current();
    if (!ctx->hasError || ctx->depth == 0) {
        return NULL;
    }
    return &ctx->errorStack[ctx->depth - 1];
}

static inline void error_clear(void) {
    ErrorContext* ctx = error_ctx_current();
    ctx->hasError = false;
    ctx->depth = 0;
    ctx->deferredError = ERROR_OK;
}

static inline ErrorCode error_pop(void) {
    ErrorContext* ctx = error_ctx_current();
    if (!ctx->hasError || ctx->depth == 0) {
        return ERROR_OK;
    }
    ErrorCode code = ctx->errorStack[--ctx->depth].code;
    if (ctx->depth == 0) {
        ctx->hasError = false;
    }
    return code;
}

static inline void error_pop_n(Uint32 count) {
    ErrorContext* ctx = error_ctx_current();
    Uint32 depth = ctx->depth;
    if (count >= depth) {
        error_clear();
    } else {
        ctx->depth -= count;
        if (ctx->depth == 0) {
            ctx->hasError = false;
        }
    }
}

static inline void error_roll_back_to(Uint32 target_depth) {
    ErrorContext* ctx = error_ctx_current();
    if (target_depth >= ctx->depth) {
        return;
    }
    ctx->depth = target_depth;
    if (target_depth == 0) {
        ctx->hasError = false;
    }
}

#define ERROR_DEFER(code) do { \
    ErrorContext* _ctx = error_ctx_current(); \
    _ctx->deferredError = (code); \
} while(0)

#define ERROR_DEFER_IF(cond, code) do { \
    if (!(cond)) { \
        ErrorContext* _ctx = error_ctx_current(); \
        _ctx->deferredError = (code); \
    } \
} while(0)

static inline void error_check_deferred_impl(ErrorContext* ctx, ConstCstring file, Uint32 line) {
#if defined(CONFIG_ERROR_FRAME_DEBUG)
    error_push(ctx->deferredError, NULL, file, line);
#else
    error_push(ctx->deferredError);
#endif
}

#define ERROR_CHECK_DEFERRED(label) do { \
    ErrorContext* _ctx = error_ctx_current(); \
    if (_ctx->deferredError != ERROR_OK) { \
        ErrorCode _err = _ctx->deferredError; \
        _ctx->deferredError = ERROR_OK; \
        error_check_deferred_impl(_ctx, __FILE__, (Uint32)__LINE__); \
        goto label; \
    } \
} while(0)

static inline Uint32 errorStack_depth(void) {
    return error_ctx_current()->depth;
}

static inline ErrorFrame* error_frame_at(Uint32 index) {
    ErrorContext* ctx = error_ctx_current();
    if (index >= ctx->depth) {
        return NULL;
    }
    return &ctx->errorStack[index];
}

#if defined(CONFIG_ERROR_FRAME_DEBUG)

#define ERROR_THROW_NEW(code, label) do { \
    error_push((code), NULL, __FILE__, (Uint32)__LINE__); \
    goto label; \
} while(0)

#define ERROR_THROW_NEW_MSG(code, msg, label) do { \
    error_push((code), (msg), __FILE__, (Uint32)__LINE__); \
    goto label; \
} while(0)

#define ERROR_THROW_NEW_IF(cond, code, label) do { \
    if (!(cond)) { \
        error_push((code), NULL, __FILE__, (Uint32)__LINE__); \
        goto label; \
    } \
} while(0)

#else

#define ERROR_THROW_NEW(code, label) do { \
    error_push((code)); \
    goto label; \
} while(0)

#define ERROR_THROW_NEW_MSG(code, msg, label) do { \
    error_push((code)); \
    goto label; \
} while(0)

#define ERROR_THROW_NEW_IF(cond, code, label) do { \
    if (!(cond)) { \
        error_push((code)); \
        goto label; \
    } \
} while(0)

#endif

#define ERROR_ENSURE(expr, code, label) \
    ERROR_THROW_NEW_IF((expr), (code), (label))

#define CHECK_ERROR(label) do { \
    if (error_pending()) { \
        goto label; \
    } \
} while(0)

#define ERROR_MAYBE_RECOVERABLE(err) \
    ({ \
        ErrorCode _e = (err); \
        Uint8 _class = ERROR_GET_CLASS(_e); \
        (_class == ERROR_CLASS_TIMEOUT || \
         _class == ERROR_CLASS_BUSY || \
         _class == ERROR_CLASS_CANCEL); \
    })

static inline void isr_error_record(Uint16 irq_num, ErrorCode code) {
    ATOMIC_STORE(&isr_error_context.lastIRQnumber, irq_num);
    ATOMIC_STORE(&isr_error_context.pendingError, code);
}

static inline ErrorCode isr_error_pending_get_and_clear(void) {
    return ATOMIC_EXCHANGE_N(&isr_error_context.pendingError, ERROR_OK);
}

static inline void isr_exit_hook(void) {
    ErrorCode pending = isr_error_pending_get_and_clear();
    
    if (pending != ERROR_OK) {
        Uint16 irq_num = ATOMIC_LOAD(&isr_error_context.lastIRQnumber);
        debug_printf("[ISR ERROR] IRQ%d: %08X\n", irq_num, pending);
        // TODO: Further process
    }
}

#endif // __LIB_ERROR_POSIX_H

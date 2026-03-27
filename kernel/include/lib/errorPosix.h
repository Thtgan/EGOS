/**
 * @file errorPosix.h
 * @brief POSIX-style Error Handling Framework for EGOS Kernel
 *
 * This header implements a thread-local error handling mechanism inspired by
 * POSIX errno semantics, but with enhanced features including:
 * - Thread-local error context (each thread maintains its own error state)
 * - Error stack for tracking multiple errors and call chain debugging
 * - Deferred error mechanism for conditional error propagation
 * - ISR-safe error recording using atomic operations
 *
 * @design Philosophy
 *
 * Unlike traditional errno which is a single global variable, this framework:
 * 1. Maintains per-thread error context, eliminating race conditions
 * 2. Supports an error stack (CONFIG_ERROR_STACK_SIZE frames) for debugging
 * 3. Provides deferred errors that can be checked and converted to real errors
 * 4. Integrates with the scheduler to switch error contexts on thread switches
 *
 * @threadSafety
 * - Thread-local operations are safe without external synchronization
 * - ISR operations use atomic primitives for interrupt safety
 * - The global context (error_global_context) is used by ISRs and boot code
 *
 * @configuration
 * - CONFIG_ERROR_FRAME_DEBUG: Enable file/line tracking in error frames
 * - CONFIG_ERROR_STACK_SIZE: Maximum depth of error stack (default: 4)
 *
 * @example Basic Usage
 * ```c
 * void* allocate_buffer(size_t size) {
 *     void* buf = kmalloc(size);
 *     ERROR_THROW_NEW_IF(buf == NULL, ERROR_OUT_OF_MEMORY, error_out);
 *     return buf;
 * error_out:
 *     return NULL;
 * }
 * ```
 *
 * @example Deferred Error Pattern
 * ```c
 * void process_data() {
 *     ERROR_DEFER_IF(validate_input(), ERROR_INVALID_ARGUMENT);
 *     ERROR_CHECK_DEFERRED(error_out);
 *     // Continue processing...
 * error_out:
 *     cleanup();
 * }
 * ```
 *
 * @see errorCode.h For ErrorCode type and error code definitions
 */
#ifndef __LIB_ERROR_POSIX_H
#define __LIB_ERROR_POSIX_H

#include <debug.h>
#include <kit/atomic.h>
#include <kit/config.h>
#include <kit/types.h>
#include <lib/errorCode.h>
#include <memory/memory.h>

/** @brief Forward declaration to break circular dependency with thread module */
typedef struct Thread Thread;
extern Thread* schedule_getCurrentThread();

#ifndef CONFIG_ERROR_STACK_SIZE
#define CONFIG_ERROR_STACK_SIZE     4
#endif

#if defined(CONFIG_ERROR_FRAME_DEBUG)

/**
 * @brief Error frame with debug information (file and line number).
 *
 * This variant is used when CONFIG_ERROR_FRAME_DEBUG is enabled, providing
 * source location tracking for debugging purposes. Each error pushed onto
 * the stack records where it was thrown.
 */
typedef struct ErrorFrame {
    ErrorCode code;      ///< The error code (see errorCode.h)
    ConstCstring file;   ///< Source file where error was raised
    Uint32 line;         ///< Line number in source file
} __attribute__((packed)) ErrorFrame;

#else

/**
 * @brief Minimal error frame without debug information.
 *
 * This variant is used when CONFIG_ERROR_FRAME_DEBUG is disabled,
 * reducing memory footprint at the cost of debugging capability.
 */
typedef struct ErrorFrame {
    ErrorCode code;      ///< The error code (see errorCode.h)
} ErrorFrame;

#endif

/**
 * @brief Thread-local error context.
 *
 * Each thread maintains its own ErrorContext, which is switched by the
 * scheduler during context switches. This ensures that errors are isolated
 * per-thread, similar to how POSIX errno works but with stack support.
 *
 * The context contains:
 * - A circular buffer (errorStack) for storing error frames
 * - Depth tracking for stack operations
 * - A deferred error slot for conditional error propagation
 */
typedef struct ErrorContext {
    bool hasError;           ///< True if there is a pending error
    Uint8 depth;             ///< Current number of errors in the stack
    Uint8 padding[2];        ///< Alignment padding
    
    ErrorFrame* stackTop;    ///< Pointer to top of error stack buffer
    ErrorFrame* errorStack;  ///< Base pointer to error stack buffer
    
    ErrorCode deferredError; ///< Deferred error (not yet pushed to stack)
} __attribute__((packed)) ErrorContext;

/**
 * @brief ISR-specific error context for interrupt handlers.
 *
 * ISRs cannot use the regular thread-local error mechanism since they
 * run outside normal thread context. This structure uses atomic operations
 * to safely record errors from interrupt context.
 *
 * The pending error is checked and cleared by isr_exit_hook() after
 * ISR execution completes.
 */
typedef struct IsrErrorContext {
    volatile ErrorCode pendingError;  ///< Error recorded by last ISR
    volatile Uint16 lastIRQnumber;    ///< IRQ number that caused the error
} __attribute__((packed)) IsrErrorContext;

/**
 * @brief Global error context used by ISRs and system-wide operations.
 *
 * This context is shared across all threads and should only be accessed
 * with appropriate synchronization. Primarily used for:
 * - System-level errors that affect the entire kernel
 * - Error reporting from interrupt handlers (via isr_error_context)
 */
extern ErrorContext error_global_context;

/**
 * @brief Boot-phase error context.
 *
 * Used during system initialization before the scheduler and thread
 * mechanism are fully operational. After boot, this may be repurposed
 * or left unused depending on configuration.
 */
extern ErrorContext error_boot_context;

/**
 * @brief ISR-specific error context for atomic error recording.
 *
 * ISRs use this context to record errors without blocking interrupts.
 * The pending error is retrieved and cleared by isr_exit_hook().
 */
extern IsrErrorContext isr_error_context;

/**
 * @brief Get the current thread's error context.
 *
 * Returns the error context associated with the currently running thread.
 * If no thread is active (e.g., during boot or in ISR), returns an
 * appropriate fallback context (boot_context or global_context).
 *
 * @return Pointer to the current ErrorContext
 */
ErrorContext* error_ctx_current();

/**
 * @brief Initialize an ErrorContext structure.
 *
 * Initializes all fields of the ErrorContext to their default values:
 * - hasError: false (no pending error)
 * - depth: 0 (empty error stack)
 * - padding: zeros (alignment)
 * - stackTop: points to error_stack array (or NULL if not provided)
 * - errorStack: points to error_stack array (or NULL if not provided)
 * - deferredError: ERROR_OK (no deferred error)
 *
 * This function should be called when creating a new thread or resetting
 * an existing ErrorContext.
 *
 * @param ctx Pointer to the ErrorContext to initialize (must not be NULL)
 * @param error_stack Pointer to the ErrorFrame array that will serve as the error stack buffer.
 *                    Can be NULL for contexts like error_global_context that don't have a
 *                    dedicated error stack buffer.
 */
void error_ctx_init(ErrorContext* ctx, ErrorFrame* error_stack);

#if defined(CONFIG_ERROR_FRAME_DEBUG)
/**
 * @brief Push an error onto the current thread's error stack (debug version).
 *
 * Records the error with source location information for debugging.
 * If the error stack is full, oldest entries are shifted out (FIFO).
 *
 * @param code The error code to record
 * @param message Optional error message string
 * @param file Source file where error occurred
 * @param line Line number in source file
 */
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

/**
 * @brief Push an error onto the current thread's error stack (minimal version).
 *
 * Records only the error code without source location information.
 * If the error stack is full, oldest entries are shifted out (FIFO).
 * 
 * DO NOT USE THIS EXPLICITLY, USE WRAPPER MACRO ERROR_THROW_* INSTEAD
 *
 * @param code The error code to record
 */
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

/**
 * @brief Check if there is a pending error in the current context.
 * 
 * @return true if an error has been recorded, false otherwise
 */
static inline bool error_pending() {
    return error_ctx_current()->hasError;
}

/**
 * @brief Get the most recent error code without removing it from stack.
 *
 * This is a peek operation that does not modify the error stack.
 *
 * @return The topmost error code, or ERROR_OK if no error pending
 */
static inline ErrorCode error_get_code() {
    ErrorContext* ctx = error_ctx_current();
    if (!ctx->hasError || ctx->depth == 0) {
        return ERROR_OK;
    }
    return ctx->errorStack[ctx->depth - 1].code;
}

/**
 * @brief Get a pointer to the most recent error frame.
 *
 * Returns a pointer to the topmost ErrorFrame on the stack, allowing
 * access to debug information (file/line) if CONFIG_ERROR_FRAME_DEBUG is enabled.
 *
 * @return Pointer to the topmost ErrorFrame, or NULL if no error pending
 */
static inline ErrorFrame* error_get_frame() {
    ErrorContext* ctx = error_ctx_current();
    if (!ctx->hasError || ctx->depth == 0) {
        return NULL;
    }
    return &ctx->errorStack[ctx->depth - 1];
}

/**
 * @brief Clear all errors from the current context.
 *
 * Resets the error stack depth to zero, clears the hasError flag,
 * and resets any deferred error. This is equivalent to clearing errno
 * in POSIX systems.
 */
static inline void error_clear() {
    ErrorContext* ctx = error_ctx_current();
    ctx->hasError = false;
    ctx->depth = 0;
    ctx->deferredError = ERROR_OK;
}

/**
 * @brief Pop and return the most recent error code.
 *
 * Removes the topmost error from the stack and returns its code.
 * If the stack becomes empty after this operation, hasError is cleared.
 *
 * @return The popped error code, or ERROR_OK if no error pending
 */
static inline ErrorCode error_pop() {
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

/**
 * @brief Pop multiple errors from the stack.
 *
 * Removes up to 'count' errors from the top of the error stack.
 * If count >= current depth, clears all errors.
 *
 * @param count Number of errors to pop
 */
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

/**
 * @brief Roll back the error stack to a specific depth.
 *
 * Truncates the error stack to the specified depth, discarding any
 * errors above that level. Useful for implementing transaction-like
 * error handling where you want to revert to a known state.
 *
 * @param target_depth The desired stack depth (0 to clear all)
 */
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

/**
 * @brief Set a deferred error without immediately throwing it.
 *
 * A deferred error is stored in the context but not pushed to the
 * error stack until ERROR_CHECK_DEFERRED is called. This allows for
 * conditional error accumulation before committing to an error state.
 *
 * @param code The error code to defer
 *
 * @example
 * ```c
 * if (!validate_input()) {
 *     ERROR_DEFER(ERROR_INVALID_ARGUMENT);
 * }
 * // Later...
 * ERROR_CHECK_DEFERRED(error_out);
 * ```
 */
#define ERROR_DEFER(code) do { \
    ErrorContext* _ctx = error_ctx_current(); \
    _ctx->deferredError = (code); \
} while(0)

/**
 * @brief Conditionally set a deferred error.
 *
 * Sets the deferred error only if the condition is false. This is
 * useful for validation checks where you want to defer error handling
 * until after all validations are complete.
 *
 * @param cond The condition to check (error deferred if false)
 * @param code The error code to defer if condition fails
 *
 * @example
 * ```c
 * ERROR_DEFER_IF(ptr != NULL, ERROR_INVALID_ARGUMENT);
 * ERROR_DEFER_IF(size > 0, ERROR_INVALID_ARGUMENT);
 * ERROR_CHECK_DEFERRED(error_out);
 * ```
 */
#define ERROR_DEFER_IF(cond, code) do { \
    if (!(cond)) { \
        ErrorContext* _ctx = error_ctx_current(); \
        _ctx->deferredError = (code); \
    } \
} while(0)

/**
 * @brief Internal helper to push a deferred error onto the stack.
 *
 * This function converts a deferred error into a real error by pushing
 * it onto the error stack. It is called by ERROR_CHECK_DEFERRED macro.
 *
 * @param ctx The error context containing the deferred error
 * @param file Source file for debug information
 * @param line Line number for debug information
 */
static inline void error_check_deferred_impl(ErrorContext* ctx, ConstCstring file, Uint32 line) {
#if defined(CONFIG_ERROR_FRAME_DEBUG)
    error_push(ctx->deferredError, NULL, file, line);
#else
    error_push(ctx->deferredError);
#endif
}

/**
 * @brief Check for deferred errors and jump to error handler.
 *
 * If a deferred error is pending, this macro:
 * 1. Clears the deferred error slot
 * 2. Pushes it onto the error stack as a real error
 * 3. Jumps to the specified label for error handling
 *
 * This allows accumulating multiple validation checks before committing
 * to an error state.
 *
 * @param label The error handler label to jump to
 *
 * @example
 * ```c
 * void process() {
 *     ERROR_DEFER_IF(validate_a(), ERROR_INVALID_ARGUMENT);
 *     ERROR_DEFER_IF(validate_b(), ERROR_INVALID_ARGUMENT);
 *     ERROR_CHECK_DEFERRED(error_out);  // Jump if any validation failed
 *     // Normal processing...
 *     return;
 * error_out:
 *     cleanup();
 * }
 * ```
 */
#define ERROR_CHECK_DEFERRED(label) do { \
    ErrorContext* _ctx = error_ctx_current(); \
    if (_ctx->deferredError != ERROR_OK) { \
        ErrorCode _err = _ctx->deferredError; \
        _ctx->deferredError = ERROR_OK; \
        error_check_deferred_impl(_ctx, __FILE__, (Uint32)__LINE__); \
        goto label; \
    } \
} while(0)

/**
 * @brief Get the current depth of the error stack.
 *
 * Returns the number of errors currently stored in the error stack.
 * Useful for debugging and implementing transaction-like rollback.
 *
 * @return Number of errors on the stack
 */
static inline Uint32 errorStack_depth() {
    return error_ctx_current()->depth;
}

/**
 * @brief Get a pointer to an error frame at a specific index.
 *
 * Allows random access into the error stack for debugging purposes.
 * Index 0 is the oldest error, depth-1 is the most recent.
 *
 * @param index The index of the error frame (0-based)
 * @return Pointer to the ErrorFrame, or NULL if index out of bounds
 */
static inline ErrorFrame* error_frame_at(Uint32 index) {
    ErrorContext* ctx = error_ctx_current();
    if (index >= ctx->depth) {
        return NULL;
    }
    return &ctx->errorStack[index];
}

#if defined(CONFIG_ERROR_FRAME_DEBUG)

/**
 * @brief Push an error and jump to error handler (debug version).
 *
 * Records the error with source location information, then jumps
 * to the specified error handler label.
 *
 * @param code The error code to throw
 * @param label The error handler label to jump to
 */
#define ERROR_THROW_NEW(code, label) do { \
    error_push((code), NULL, __FILE__, (Uint32)__LINE__); \
    goto label; \
} while(0)

/**
 * @brief Push an error with message and jump to handler (debug version).
 *
 * Records the error with a custom message string and source location,
 * then jumps to the specified error handler label.
 *
 * @param code The error code to throw
 * @param msg Custom error message string
 * @param label The error handler label to jump to
 */
#define ERROR_THROW_NEW_MSG(code, msg, label) do { \
    error_push((code), (msg), __FILE__, (Uint32)__LINE__); \
    goto label; \
} while(0)

/**
 * @brief Conditionally throw an error and jump to handler (debug version).
 *
 * If the condition is false, records the error with source location
 * and jumps to the specified error handler label.
 *
 * @param cond The condition to check (error thrown if false)
 * @param code The error code to throw
 * @param label The error handler label to jump to
 */
#define ERROR_THROW_NEW_IF(cond, code, label) do { \
    if ((cond)) { \
        error_push((code), NULL, __FILE__, (Uint32)__LINE__); \
        goto label; \
    } \
} while(0)

#else

/**
 * @brief Push an error and jump to handler (minimal version).
 *
 * Records only the error code without source location, then jumps
 * to the specified error handler label.
 *
 * @param code The error code to throw
 * @param label The error handler label to jump to
 */
#define ERROR_THROW_NEW(code, label) do { \
    error_push((code)); \
    goto label; \
} while(0)

/**
 * @brief Push an error with message and jump to handler (minimal version).
 *
 * Records only the error code without source location or message,
 * then jumps to the specified error handler label.
 * Note: The msg parameter is ignored in minimal mode.
 *
 * @param code The error code to throw
 * @param msg Custom error message (ignored in minimal mode)
 * @param label The error handler label to jump to
 */
#define ERROR_THROW_NEW_MSG(code, msg, label) do { \
    error_push((code)); \
    goto label; \
} while(0)

/**
 * @brief Conditionally throw an error and jump to handler (minimal version).
 *
 * If the condition is true, records only the error code without
 * source location, then jumps to the specified error handler label.
 *
 * @param cond The condition to check (error thrown if true)
 * @param code The error code to throw
 * @param label The error handler label to jump to
 */
#define ERROR_THROW_NEW_IF(cond, code, label) do { \
    if ((cond)) { \
        error_push((code)); \
        goto label; \
    } \
} while(0)

#endif

/**
 * @brief Ensure an expression is true, otherwise throw error.
 *
 * This is a convenience macro that throws an error if the given
 * expression evaluates to false. It's semantically equivalent to
 * ERROR_THROW_NEW_IF but with more intuitive parameter ordering.
 *
 * @param expr The expression that must be true
 * @param code The error code to throw if expression is false
 * @param label The error handler label to jump to
 *
 * @example
 * ```c
 * ERROR_ENSURE(ptr != NULL, ERROR_INVALID_ARGUMENT, error_out);
 * ERROR_ENSURE(size < MAX_SIZE, ERROR_INVALID_ARGUMENT, error_out);
 * ```
 */
#define ERROR_ENSURE(expr, code, label) \
    ERROR_THROW_NEW_IF(!(expr), (code), (label))

/**
 * @brief Check for any pending error and jump to handler.
 *
 * If there is any pending error in the current context, jumps to
 * the specified error handler label. This is useful after calling
 * functions that may set errors but don't throw directly.
 *
 * @param label The error handler label to jump to if error pending
 *
 * @example
 * ```c
 * some_function_that_may_fail();
 * CHECK_ERROR(error_out);
 * // Continue only if no error
 * ```
 */
#define CHECK_ERROR(label) do { \
    if (error_pending()) { \
        goto label; \
    } \
} while(0)

/**
 * @brief Check if an error is potentially recoverable by retrying.
 *
 * Returns true if the error class indicates a transient condition
 * that might succeed on retry:
 * - TIMEOUT: Operation timed out, may succeed with more time
 * - BUSY: Resource temporarily unavailable, may become available
 * - CANCEL: Operation was cancelled, can be restarted
 *
 * @param err The error code to check
 * @return true if the error is potentially recoverable, false otherwise
 *
 * @example
 * ```c
 * ErrorCode err = connect_socket(addr);
 * while (err != ERROR_OK && ERROR_MAYBE_RECOVERABLE(err)) {
 *     sleep(100);  // Wait before retry
 *     err = connect_socket(addr);
 * }
 * ```
 */
#define ERROR_MAYBE_RECOVERABLE(err) \
    ({ \
        ErrorCode _e = (err); \
        Uint8 _class = ERROR_GET_CLASS(_e); \
        (_class == ERROR_CLASS_TIMEOUT || \
         _class == ERROR_CLASS_BUSY || \
         _class == ERROR_CLASS_CANCEL); \
    })

/**
 * @brief Record an error from ISR context.
 *
 * Atomically stores the IRQ number and error code in the ISR-specific
 * error context. This function is safe to call from interrupt handlers
 * as it uses atomic operations.
 *
 * The recorded error can later be retrieved by isr_error_pending_get_and_clear()
 * or automatically logged by isr_exit_hook().
 *
 * @param irq_num The IRQ number that encountered the error
 * @param code The error code to record
 */
static inline void isr_error_record(Uint16 irq_num, ErrorCode code) {
    ATOMIC_STORE(&isr_error_context.lastIRQnumber, irq_num);
    ATOMIC_STORE(&isr_error_context.pendingError, code);
}

/**
 * @brief Atomically get and clear any pending ISR error.
 *
 * Performs an atomic exchange operation that:
 * 1. Returns the current pending error value
 * 2. Sets the pending error to ERROR_OK (clearing it)
 *
 * This is a lock-free, interrupt-safe operation suitable for retrieving
 * errors recorded by ISR handlers.
 *
 * @return The previously pending error code, or ERROR_OK if none
 */
static inline ErrorCode isr_error_pending_get_and_clear() {
    return ATOMIC_EXCHANGE_N(&isr_error_context.pendingError, ERROR_OK);
}

/**
 * @brief ISR exit hook for automatic error logging.
 *
 * This function should be called at the end of ISR execution to:
 * 1. Check if any error was recorded during ISR handling
 * 2. Log the error with IRQ number for debugging
 * 3. Clear the pending error state
 *
 * The hook provides a centralized place for ISR error reporting without
 * requiring each individual ISR handler to handle errors explicitly.
 *
 * @note Currently only logs errors via debug_printf(). Future enhancement
 *       may include more sophisticated error handling mechanisms.
 */
static inline void isr_exit_hook() {
    ErrorCode pending = isr_error_pending_get_and_clear();
    
    if (pending != ERROR_OK) {
        Uint16 irq_num = ATOMIC_LOAD(&isr_error_context.lastIRQnumber);
        debug_printf("[ISR ERROR] IRQ%d: %08X\n", irq_num, pending);
        // TODO: Further process
    }
}

#endif // __LIB_ERROR_POSIX_H

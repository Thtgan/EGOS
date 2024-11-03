#include<multitask/criticalToken.h>

#include<debug.h>
#include<kit/atomic.h>
#include<kit/types.h>
#include<interrupt/IDT.h>
#include<multitask/process.h>
#include<multitask/schedule.h>
#include<multitask/spinlock.h>

#define __CRITICAL_TOKEN_IS_ACCESSED_BY_HOLDER(__TOKEN, __TOKEN_ID) (ATOMIC_BARRIER_READ((__TOKEN)->tokenID) == (__TOKEN_ID) && ATOMIC_BARRIER_READ((__TOKEN)->depth) > 0)
#define __CRITICAL_TOKEN_SET_HOLDER(__TOKEN, __TOKEN_ID) do {   \
    ATOMIC_BARRIER_WRITE((__TOKEN)->depth, 1);                  \
    ATOMIC_BARRIER_WRITE((__TOKEN)->tokenID, (__TOKEN_ID));     \
} while (0);

#define __CRITICAL_TOKEN_CLEAR_HOLDER(__TOKEN) do {         \
    ATOMIC_BARRIER_WRITE((__TOKEN)->depth, 0);              \
    ATOMIC_BARRIER_WRITE((__TOKEN)->tokenID, OBJECT_NULL);  \
} while (0);

void criticalToken_initStruct(CriticalToken* token) {
    token->interruptEnabled = false;
    token->depth = 0;
    token->waitCount = 0;
    token->waitLock = SPINLOCK_LOCKED;
    token->lock = SPINLOCK_UNLOCKED;
    token->tokenID = OBJECT_NULL;
}

void criticalToken_enter(CriticalToken* token, Object tokenID) {
    if (__CRITICAL_TOKEN_IS_ACCESSED_BY_HOLDER(token, tokenID)) {
        ++token->depth;
        return;
    }
    
    bool interruptEnabled = idt_disableInterrupt();
    ATOMIC_INC_FETCH(&token->waitCount);
    while (true) {
        if (spinlock_tryLock(&token->lock)) {
            ATOMIC_DEC_FETCH(&token->waitCount);
            token->interruptEnabled = interruptEnabled;
            __CRITICAL_TOKEN_SET_HOLDER(token, tokenID);
            break;
        } else {
            idt_setInterrupt(interruptEnabled);
            spinlock_lock(&token->waitLock);
            idt_disableInterrupt();
        }
    }
}

bool criticalToken_tryEnter(CriticalToken* token, Object tokenID) {
    if (__CRITICAL_TOKEN_IS_ACCESSED_BY_HOLDER(token, tokenID)) {
        ++token->depth;
        return true;
    }

    bool interruptEnabled = idt_disableInterrupt();
    if (!spinlock_tryLock(&token->lock)) {
        idt_setInterrupt(interruptEnabled);
        return false;
    }

    token->interruptEnabled = interruptEnabled;
    __CRITICAL_TOKEN_SET_HOLDER(token, tokenID);

    return true;
}

Result criticalToken_leave(CriticalToken* token, Object tokenID) {
    if (!__CRITICAL_TOKEN_IS_ACCESSED_BY_HOLDER(token, tokenID)) {
        return RESULT_FAIL;
    }

    if (token->depth == 1) {
        __CRITICAL_TOKEN_CLEAR_HOLDER(token);
        idt_setInterrupt(token->interruptEnabled);

        spinlock_unlock(&token->lock);
        if (token->waitCount > 0) {
            spinlock_unlock(&token->waitLock);
        }
    } else {
        --token->depth;
    }

    return RESULT_SUCCESS;
}
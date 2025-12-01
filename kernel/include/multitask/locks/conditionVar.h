#if !defined(__MULTITASK_LOCKS_CONDITIONVAR_H)
#define __MULTITASK_LOCKS_CONDITIONVAR_H

typedef struct ConditionVar ConditionVar;

#include<multitask/wait.h>
#include<multitask/locks/mutex.h>

typedef struct ConditionVar {
    Wait wait;
} ConditionVar;

typedef bool (*ConditionVariableFunc)(void* arg);

void conditionVar_initStruct(ConditionVar* cond);

void conditionVar_wait(ConditionVar* cond, Mutex* lock, ConditionVariableFunc func, void* arg);

void conditionVar_waitOnce(ConditionVar* cond, Mutex* lock);

void conditionVar_notify(ConditionVar* cond);

void conditionVar_notifyAll(ConditionVar* cond);

#endif // __MULTITASK_LOCKS_CONDITIONVAR_H

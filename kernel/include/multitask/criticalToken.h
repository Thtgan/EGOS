#if !defined(__MULTITASK_CRITICALSECTION_H)
#define __MULTITASK_CRITICALSECTION_H

typedef struct CriticalToken CriticalToken;

#include<kit/types.h>
#include<interrupt/IDT.h>
#include<multitask/spinlock.h>

typedef struct CriticalToken {
    bool interruptEnabled;
    int depth;
    int waitCount;
    Spinlock waitLock;
    Spinlock lock;
    Object tokenID;
} CriticalToken;

void criticalToken_initStruct(CriticalToken* token);

void criticalToken_enter(CriticalToken* token, Object tokenID);

bool criticalToken_tryEnter(CriticalToken* token, Object tokenID);

Result criticalToken_leave(CriticalToken* token, Object tokenID);

#endif // __MULTITASK_CRITICALSECTION_H

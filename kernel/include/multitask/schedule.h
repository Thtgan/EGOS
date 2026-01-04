#if !defined(__MULTITASK_SCHEDULE_H)
#define __MULTITASK_SCHEDULE_H

#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/thread.h>
#include<test.h>

void schedule_setEarlyStackBottom(void* stackBottom);

void schedule_init();

Uint16 schedule_allocateNewID();

void schedule_releaseID(Uint16 id);

void schedule_tick();

bool schedule_yield();

void schedule_isrDelayYield();

void schedule_addProcess(Process* process);

void schedule_removeProcess(Process* process);

void schedule_addThread(Thread* thread);

void schedule_removeThread(Thread* thread);

void schedule_threadJoinSchedule(Thread* thread);

void schedule_threadQuitSchedule(Thread* thread);

Process* schedule_getCurrentProcess();

Process* schedule_getProcessFromPID(Uint16 pid);

Thread* schedule_getCurrentThread();

Thread* schedule_getThreadFromTID(Uint16 tid);

void schedule_enterCritical();

void schedule_leaveCritical();

bool schedule_isInCritical();

void schedule_yieldIfStopped();

Process* schedule_fork();

void schedule_collectOrphans(Process* process);

#if defined(CONFIG_UNIT_TEST_SCHEDULE)
TEST_EXPOSE_GROUP(schedule_testGroup);
#define UNIT_TEST_GROUP_SCHEDULE    &schedule_testGroup
#else
#define UNIT_TEST_GROUP_SCHEDULE    NULL
#endif

#endif // __MULTITASK_SCHEDULE_H

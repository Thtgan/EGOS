#if !defined(__MULTITASK_REAPER_H)
#define __MULTITASK_REAPER_H

#include<multitask/thread.h>

void reaper_init();

void reaper_daemon();

void reaper_submitThread(Thread* thread);

#endif // __MULTITASK_REAPER_H

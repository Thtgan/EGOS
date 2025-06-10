#if !defined(__REAPER_H)
#define __REAPER_H

#include<multitask/thread.h>

void reaper_init();

void reaper_daemon();

void reaper_submitThread(Thread* thread);

#endif // __REAPER_H

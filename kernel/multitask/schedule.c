#include<multitask/schedule.h>

#include<debug.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/simpleSchedule.h>

static void __idle();

static Scheduler* _scheduler = NULL;

Result initSchedule() {
    _scheduler = createSimpleScheduler();

    schedulerStart(initProcess());

    if (fork("Idle") == NULL) {
        __idle();
    }

    return RESULT_SUCCESS;
}

Scheduler* getScheduler() {
    return _scheduler;
}

static void __idle() {
    while (true) {
        sti();
        hlt();
        //No, no yield here, currently it will make system freezed for input
    }

    debug_belowup("Idle is trying to return\n");
}
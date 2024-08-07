#include<multitask/schedule.h>

#include<debug.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/simpleSchedule.h>

static void __schedule_idle();

static Scheduler* _schedule_currentScheduler = NULL;

Result schedule_init() {
    _schedule_currentScheduler = simpleScheduler_create();

    schedulerStart(process_init());

    if (fork("Idle") == NULL) {
        __schedule_idle();
    }

    return RESULT_SUCCESS;
}

Scheduler* schedule_getCurrentScheduler() {
    return _schedule_currentScheduler;
}

static void __schedule_idle() {
    while (true) {
        sti();
        hlt();
        //No, no yield here, currently it will make system freezed for input
    }

    debug_blowup("Idle is trying to return\n");
}
#include<multitask/schedule.h>

#include<debug.h>
#include<interrupt/IDT.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<multitask/simpleSchedule.h>

static void __schedule_idle();

static Scheduler* _schedule_currentScheduler = NULL;

void schedule_init() {
    _schedule_currentScheduler = simpleScheduler_create();

    scheduler_start(_schedule_currentScheduler, process_init());

    // if (process_fork("Idle") == NULL) {
    //     __schedule_idle();
    // }
}

Scheduler* schedule_getCurrentScheduler() {
    return _schedule_currentScheduler;
}

static void __schedule_idle() {
    sti();
    while (true) {
        hlt();
        // scheduler_yield(schedule_getCurrentScheduler());
        //No, no yield here, currently it will make system freezed for input
    }

    debug_blowup("Idle is trying to return\n");
}

void scheduler_tryYield(Scheduler* scheduler) {
    if (idt_isInISR(false)) {
        scheduler->isrDelayYield = true;
    } else {
        scheduler_yield(scheduler);
    }
}

void scheduler_isrDelayYield(Scheduler* scheduler) {
    if (scheduler->isrDelayYield && !idt_isInISR(false)) {
        scheduler_yield(scheduler);
        scheduler->isrDelayYield = false;
    }
}
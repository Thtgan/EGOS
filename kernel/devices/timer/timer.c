#include<devices/timer/timer.h>

#include<devices/timer/PIT.h>
#include<kit/types.h>

Result initTimer() {
    initPIT();
    return RESULT_SUCCESS;
}

void sleep(TimeUnit unit, Uint64 time) {
    PITsleep(unit, time);
}
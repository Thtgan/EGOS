#include<devices/timer/timer.h>

#include<devices/timer/PIT.h>
#include<kit/types.h>

Result initTimer() {
    initPIT();
}

void sleep(TimeUnit unit, uint64_t time) {
    PITsleep(unit, time);
}
#if !defined(__TIMER_H)
#define __TIMER_H

#include<stdint.h>

typedef enum {
    NANOSECOND  = 1,
    MICROSECOND = NANOSECOND    * 1000,
    MILLISECOND = MICROSECOND   * 1000,
    SECOND      = MILLISECOND   * 1000
} TimeUnit;

void initTimer();

void sleep(TimeUnit unit, uint64_t time);

#endif // __TIMER_H

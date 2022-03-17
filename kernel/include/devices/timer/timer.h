#if !defined(__TIMER_H)
#define __TIMER_H

typedef enum {
    NANOSECOND  = 1,
    MICROSECOND = NANOSECOND    * 1000,
    MILLISECOND = MICROSECOND   * 1000,
    SECOND      = MILLISECOND   * 1000
} TimeUnit;

void initTimer();

#endif // __TIMER_H

#if !defined(__TIMER_H)
#define __TIMER_H

#include<stdint.h>

typedef enum {
    NANOSECOND  = 1,
    MICROSECOND = NANOSECOND    * 1000,
    MILLISECOND = MICROSECOND   * 1000,
    SECOND      = MILLISECOND   * 1000
} TimeUnit;

/**
 * @brief Initialize the timer
 */
void initTimer();

/**
 * @brief Sleep for a period of time
 * 
 * @param unit Time unit
 * @param time Time
 */
void sleep(TimeUnit unit, uint64_t time);

#endif // __TIMER_H

#if !defined(__TIME_TIME_H)
#define __TIME_TIME_H

typedef enum {
    TIME_UNIT_NANOSECOND    = 1,
    TIME_UNIT_MICROSECOND   = TIME_UNIT_NANOSECOND  * 1000,
    TIME_UNIT_MILLISECOND   = TIME_UNIT_MICROSECOND * 1000,
    TIME_UNIT_SECOND        = TIME_UNIT_MILLISECOND * 1000
} TimeUnit;

typedef struct RealTime RealTime;
typedef struct Timestamp Timestamp;

#include<kit/config.h>
#include<kit/types.h>
#include<multitask/locks/spinlock.h>
#include<test.h>

typedef struct RealTime {
    Uint16  year;
    Uint8   month;
    Uint8   day;
    Uint8   hour;
    Uint8   minute;
    Uint8   second;
    Uint16  millisecond;
    Uint16  microsecond;
    Uint16  nanosecond;
} RealTime;

typedef struct Timestamp {
    long   second;
    long   nanosecond;
} Timestamp;

void time_convertRealTimeToTimestamp(RealTime* realTime, Timestamp* timestamp);

void time_convertTimestampToRealTime(Timestamp* timestamp, RealTime* realTime);

void timestamp_step(Timestamp* timestamp, Uint64 time, TimeUnit unit);

Int64 timestamp_compare(Timestamp* ts1, Timestamp* ts2);

void time_init();

void time_getTimestamp(Timestamp* timestamp);

#if defined(CONFIG_UNIT_TEST_TIME)
TEST_EXPOSE_GROUP(time_testGroup);
#define UNIT_TEST_GROUP_TIME    &time_testGroup
#else
#define UNIT_TEST_GROUP_TIME    NULL
#endif

#endif // __TIME_TIME_H

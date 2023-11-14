#if !defined(__TIME_H)
#define __TIME_H

#include<kit/types.h>
#include<multitask/spinlock.h>

typedef enum {
    TIME_UNIT_NANOSECOND    = 1,
    TIME_UNIT_MICROSECOND   = TIME_UNIT_NANOSECOND  * 1000,
    TIME_UNIT_MILLISECOND   = TIME_UNIT_MICROSECOND * 1000,
    TIME_UNIT_SECOND        = TIME_UNIT_MILLISECOND * 1000
} TimeUnit;

typedef struct {
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

typedef struct {
    Int64   second;
    Uint32  nanosecond;
} Timestamp;

void timeConvertRealTimeToTimestamp(RealTime* realTime, Timestamp* timestamp);

void timeConvertTimestampToRealTime(Timestamp* timestamp, RealTime* realTime);

void timestampStep(Timestamp* timestamp, Uint64 time, TimeUnit unit);

Int64 timesampCompare(Timestamp* ts1, Timestamp* ts2);

Result initTime();

void readTimestamp(Timestamp* timestamp);

#endif // __TIME_H

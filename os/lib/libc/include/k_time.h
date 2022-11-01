#ifndef __K_TIME_H
#define __K_TIME_H

#include <chinos/config.h>

#include <k_stdint.h>

typedef uint64_t  time64_t;       /* Holds time in microseconds */
typedef uint32_t  time_t;         /* Holds time in seconds */
typedef uint8_t   clockid_t;      /* Identifies one time base source */
typedef void *timer_t;        /* Represents one POSIX timer */

/* struct timespec is the standard representation of time as seconds and
 * nanoseconds.
 */
struct timespec
{
    time_t tv_sec;                   /* Seconds */
    long   tv_nsec;                  /* Nanoseconds */
};

/* struct tm is the standard representation for "broken out" time.
 *
 * REVISIT: This structure could be packed better using uint8_t's and
 * uint16_t's.  The standard definition does, however, call out type int for
 * all of the members.  NOTE: Any changes to this structure must be also be
 * reflected in struct rtc_time defined in include/nuttx/timers/rtc.h; these
 * two structures must be cast compatible.
 */
struct tm
{
    int  tm_sec;         /* Seconds (0-61, allows for leap seconds) */
    int  tm_min;         /* Minutes (0-59) */
    int  tm_hour;        /* Hours (0-23) */
    int  tm_mday;        /* Day of the month (1-31) */
    int  tm_mon;         /* Month (0-11) */
    int  tm_year;        /* Years since 1900 */
    int  tm_wday;        /* Day of the week (0-6) */
    int  tm_yday;        /* Day of the year (0-365) */
    int  tm_isdst;       /* Non-0 if daylight savings time is in effect */
    long tm_gmtoff;      /* Offset from UTC in seconds */
    const char *tm_zone; /* Timezone abbreviation. */
};

/* Struct itimerspec is used to define settings for an interval timer */
struct itimerspec
{
    struct timespec it_value;    /* First time */
    struct timespec it_interval; /* and thereafter */
};

#endif

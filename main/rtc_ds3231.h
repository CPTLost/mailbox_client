#ifndef RTC_DS3231_H
#define RTC_DS3231_H

#include <sys/time.h>

void rtc_test_task(void* param);
void set_rtc_time(int year, int month, int day, int hour, int minute, int second);
time_t get_time_in_unix(void);


#endif
#ifndef SLEEP_H
#define SLEEP_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define SCALE_WAKEUP_CAUSE 0
#define BATTERY_WAKEUP_CAUSE 1
#define TIMER_WAKEUP_CAUSE 2
typedef struct
{
    TaskHandle_t gpio_wakeup_task;
    TaskHandle_t timer_wakeup_task;
} wakeup_TaskHandles_t;

void deep_sleep_task(void *param);

#endif
#ifndef UDP_CLIENT_H
#define UDP_CLIENT_H

#include "inttypes.h"
#include <sys/time.h>
#include "freertos/FreeRTOS.h" 
#include "freertos/task.h"  

typedef struct
{
    time_t wakeup_time_in_unix;
    uint8_t battery_charge_in_perc;
    uint32_t scale_weight_in_g;
    char message_buffer[128];
} udp_data_t;

typedef struct
{
    udp_data_t *udp_data;
    TaskHandle_t *sleep_TaskHandle;
} udp_client_task_param_t;


void udp_client_task(void *pvParameters);

#endif
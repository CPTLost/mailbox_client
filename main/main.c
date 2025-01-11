/*******************************************************************
 * File Name         : main.c
 * Description       : handles deep sleep and udp client tasks
 *
 * Author            : Noah Plank
 *
 ******************************************************************/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "inttypes.h"
#include <sys/time.h>
#include <string.h>

#include "wifi.h"
#include "udp_client.h"
#include "led_matrix.h"
#include "adc.h"
#include "rtc_ds3231.h"
#include "sleep.h"

#define DOOMSDAY 0
#define BLINKING_DELAY_MS 500

#define STD_STACK_SIZE 4096
#define STD_PRIO 3

static const char *TAG = "MAIN";

TaskHandle_t sleep_TaskHandle = NULL;
TaskHandle_t get_data_TaskHandle = NULL;
TaskHandle_t udp_client_TaskHandle = NULL;

RTC_DATA_ATTR static udp_data_t udp_data = {0};

/// @brief Gets notified by the deep_sleep_task when the device was woken up with a gpio interrupt
/// @param param void
void get_data_task(void *param)
{
    uint32_t notify_value = 0;
    while (!DOOMSDAY)
    {
        xTaskNotifyWait(0, ULONG_MAX, &notify_value, portMAX_DELAY);

        udp_data.battery_charge_in_perc = get_battery_charge_in_perc();
        udp_data.scale_weight_in_g = get_weight_in_g();

        printf("\nnotify_value = %ld\n",notify_value);

        if ((1 << SCALE_WAKEUP_CAUSE) & notify_value)
        {
            ESP_LOGI(TAG, "SCALE_WAKEUP_CAUSE");
            udp_data.wakeup_time_in_unix = get_time_in_unix();
            strcpy(udp_data.message_buffer, "Scale wakeup");
        }
        else if ((1 << BATTERY_WAKEUP_CAUSE) & notify_value)
        {
            ESP_LOGI(TAG, "BATTERY_WAKEUP_CAUSE");
            udp_data.wakeup_time_in_unix = get_time_in_unix();
            strcpy(udp_data.message_buffer, "Battery low!");
        }
        else if ((1 << TIMER_WAKEUP_CAUSE) & notify_value)
        {
            ESP_LOGI(TAG, "TIMER_WAKEUP_CAUSE");
            strcpy(udp_data.message_buffer, "Timer wakeup");
        }

        // Data is ready to send -> notify udp_client_task
        xTaskNotify(udp_client_TaskHandle, 0, eNoAction);
    }
}

void app_main(void)
{
    xTaskCreate(get_data_task, "get_data_task", STD_STACK_SIZE, NULL, STD_PRIO, &get_data_TaskHandle);

    udp_client_task_param_t *udp_client_task_param = malloc(sizeof(udp_client_task_param_t));
    if (NULL == udp_client_task_param)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for udp_client_task_param struct");
    }
    else
    {
        udp_client_task_param->udp_data = &udp_data;
        udp_client_task_param->sleep_TaskHandle = &sleep_TaskHandle;
        xTaskCreate(udp_client_task, "udp_server_task", STD_STACK_SIZE, udp_client_task_param, STD_PRIO, &udp_client_TaskHandle);
    }

    wakeup_TaskHandles_t *task_handles = malloc(sizeof(wakeup_TaskHandles_t));
    if (NULL == task_handles)
    {
        ESP_LOGE(TAG, "Failed to allocate memory for task_handles struct");
    }
    else
    {
        task_handles->gpio_wakeup_task = get_data_TaskHandle;
        task_handles->timer_wakeup_task = get_data_TaskHandle;

        xTaskCreate(deep_sleep_task, "deep_sleep_task", STD_STACK_SIZE, task_handles, STD_PRIO, &sleep_TaskHandle);
    }
}
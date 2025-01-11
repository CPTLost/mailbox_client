/*******************************************************************
 * File Name         : sleep.c
 * Description       : Defines the deep_sleep_task
 *
 * Author            : Noah Plank
 *
 ******************************************************************/
#include "sleep.h"

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "sdkconfig.h"
#include "soc/soc_caps.h"
#include "esp_sleep.h"
#include "esp_log.h"
#include "driver/rtc_io.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "limits.h"

#define DOOMSDAY 0

#define SCALE_WAKEUP_PIN CONFIG_GPIO_SCALE_WAKEUP_PIN
#define BATTERY_WAKEUP_PIN CONFIG_GPIO_BATTERY_WAKEUP_PIN

#ifdef CONFIG_GPIO_BATTERY_WAKEUP_HIGH_LEVEL
#define BATTERY_WAKEUP_LEVEL ESP_GPIO_WAKEUP_GPIO_HIGH
#else
#define BATTERY_WAKEUP_LEVEL ESP_GPIO_WAKEUP_GPIO_LOW
#endif

#ifdef CONFIG_GPIO_SCALE_WAKEUP_HIGH_LEVEL
#define SCALE_WAKEUP_LEVEL ESP_GPIO_WAKEUP_GPIO_HIGH
#else
#define SCALE_WAKEUP_LEVEL ESP_GPIO_WAKEUP_GPIO_LOW
#endif

#define WAKEUP_TIME_SEC 10

static RTC_DATA_ATTR struct timeval sleep_enter_time;

static const char *TAG = "SLEEP";

static void deep_sleep_register_gpio_wakeup(void)
{
    const gpio_config_t config = {
        .pin_bit_mask = BIT(SCALE_WAKEUP_PIN) | BIT(BATTERY_WAKEUP_PIN),
        .mode = GPIO_MODE_INPUT,
    };

    ESP_ERROR_CHECK(gpio_config(&config));
    ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(BIT(SCALE_WAKEUP_PIN), SCALE_WAKEUP_LEVEL));
    ESP_ERROR_CHECK(esp_deep_sleep_enable_gpio_wakeup(BIT(BATTERY_WAKEUP_PIN), BATTERY_WAKEUP_LEVEL));

    ESP_LOGI(TAG, "Enabling GPIO wakeup on pins GPIO%d\n", SCALE_WAKEUP_PIN);
    ESP_LOGI(TAG, "Enabling GPIO wakeup on pins GPIO%d\n", BATTERY_WAKEUP_PIN);
}

/// @brief Checks the wakeup causes and manages the actions accordingly
/// @param param This must be a pointer to a wakeup_TaskHandles_t struct
void deep_sleep_task(void *param)
{
    deep_sleep_register_gpio_wakeup();

    while (!DOOMSDAY)
    {
        switch (esp_sleep_get_wakeup_cause())
        {
        case ESP_SLEEP_WAKEUP_TIMER:
        {
            ESP_LOGI(TAG, "Wake up from timer\n");
            xTaskNotify(((wakeup_TaskHandles_t *)param)->timer_wakeup_task, (1 << TIMER_WAKEUP_CAUSE), eSetBits);
            break;
        }

        case ESP_SLEEP_WAKEUP_GPIO:
        {
            uint64_t wakeup_pin_mask = esp_sleep_get_gpio_wakeup_status();
            if (wakeup_pin_mask != 0)
            {
                uint32_t pin = __builtin_ffsll(wakeup_pin_mask) - 1;
                ESP_LOGI(TAG, "Wake up from GPIO %ld\n", pin);
                if (pin == SCALE_WAKEUP_PIN)
                {
                    xTaskNotify(((wakeup_TaskHandles_t *)param)->gpio_wakeup_task, (1 << SCALE_WAKEUP_CAUSE), eSetBits);
                }
                else if (pin == BATTERY_WAKEUP_PIN)
                {
                    xTaskNotify(((wakeup_TaskHandles_t *)param)->gpio_wakeup_task, (1 << BATTERY_WAKEUP_CAUSE), eSetBits);
                }
            }
            break;
        }
        case ESP_SLEEP_WAKEUP_UNDEFINED:
        default:
            ESP_LOGI(TAG, "Not a deep sleep reset, Entering deep sleep\n");
            esp_deep_sleep_start();
        }

        // If this Task is notified with a value it is inteded to go to sleep for that amount of seconds and then woken up by the timer
        // else it should only be woken up by an gpio interrupt
        uint32_t sleep_time_in_s = 0;
        ESP_LOGI(TAG, "Waiting for notify\n");
        xTaskNotifyWait(0, ULONG_MAX, &sleep_time_in_s, portMAX_DELAY);
        if (0 == sleep_time_in_s)
        {
            // sleeps ~forever -> waits for gpio interrupt
            ESP_LOGI(TAG, "Sets time to wakeup to forever -> waiting for GPIO wakeup\n");
            ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(0xFFFFFFFF));
        }
        else
        {
            ESP_LOGI(TAG, "Sets time to wakeup to %lds\n", sleep_time_in_s);
            ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(sleep_time_in_s * 1000000));
        }

        ESP_LOGW(TAG, "Entering deep sleep\n");
        esp_deep_sleep_start();
    }
}
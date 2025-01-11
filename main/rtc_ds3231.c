/*******************************************************************
 * File Name         : rtc_ds3231.c
 * Description       : Driver for the DS3231 RTC module
 *
 * Author            : Noah Plank
 *
 ******************************************************************/
#include <stdio.h>
#include "esp-idf-ds3231.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#define SCL_PIN CONFIG_SCL_PIN
#define SDA_PIN CONFIG_SDA_PIN

static const char *TAG = "RTC_DS3231";

static i2c_master_bus_handle_t *bus_handle = NULL;
static rtc_handle_t *rtc_handle = NULL;

static void init_i2c_bus()
{
    // Allocte memory for the pointer of i2c_master_bus_handle_t
    i2c_master_bus_handle_t *new_bus_handle =
        (i2c_master_bus_handle_t *)malloc(sizeof(i2c_master_bus_handle_t));
    // Create the i2c_master_bus_config_t struct and assign values.
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = SCL_PIN,
        .sda_io_num = SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    i2c_new_master_bus(&i2c_mst_config, new_bus_handle);
    bus_handle = new_bus_handle;
}

time_t get_time_in_unix(void)
{
    if (NULL == bus_handle)
    {
        init_i2c_bus();
    }
    if (NULL == rtc_handle)
    {
        rtc_handle = ds3231_init(bus_handle);
    }

    return ds3231_time_unix_get(rtc_handle);
}

/// @brief used to set up the DS3231 RTC module time only ONCE
void set_rtc_time(int year, int month, int day, int hour, int minute, int second)
{
    if (year >= 1900 &&
        month < 12 && month >= 0 &&
        day <= 31 && day > 0 &&
        hour <= 24 && hour >= 0 &&
        minute <= 60 && minute >= 0 &&
        second <= 60 && second >= 0)
    {
        if (NULL == bus_handle)
        {
            init_i2c_bus();
        }
        if (NULL == rtc_handle)
        {
            rtc_handle = ds3231_init(bus_handle);
        }
        rtc_handle_t *rtc_handle = ds3231_init(bus_handle);
        struct tm time_to_set = {
            .tm_year = year - 1900, // Year since 1900
            .tm_mon = month,        // Month, where 0 = January
            .tm_mday = day,         // Day of the month
            .tm_hour = hour,        // Hours since midnight
            .tm_min = minute,       // Minutes after the hour
            .tm_sec = second        // Seconds after the minute
        };
        ds3231_time_tm_set(rtc_handle, time_to_set);
    }
    else
    {
        ESP_LOGE(TAG, "Invalid time values");
    }
}

/*******************************************************************
 * File Name         : adc.c
 * Description       : calibrates and reads defined ADC1 Channels
 *
 * Author            : Noah Plank
 *
 ******************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

#define DOOMSDAY 0

#define ADC1_CHAN_SCALE CONFIG_ADC1_CHAN_SCALE
#define ADC1_CHAN_BATTERY CONFIG_ADC1_CHAN_BATTERY

#define WAITING_TIME_BEFORE_ADC_READ_IN_S CONFIG_WAITING_TIME_BEFORE_ADC_READ_IN_S

#define ADC_ATTEN ADC_ATTEN_DB_12

#define SCALE_FLOAT_CONSTANT_IN_G_PER_MV 2.3036
#define LINEAR_OFFSET_IN_G 3080

static const float scale_constant_in_g_per_mv = SCALE_FLOAT_CONSTANT_IN_G_PER_MV;

#define ADC_VALUE_BUFFER_SIZE 10

const static char *TAG = "ADC";

static int adc_raw_scale[ADC_VALUE_BUFFER_SIZE];
static int adc_raw_battery[ADC_VALUE_BUFFER_SIZE];
static int scale_voltage_in_mv[ADC_VALUE_BUFFER_SIZE];
static int battery_voltage_in_mv[ADC_VALUE_BUFFER_SIZE];

static adc_oneshot_unit_handle_t adc1_handle = NULL;
adc_cali_handle_t adc1_cali_chan_scale_handle = NULL;
adc_cali_handle_t adc1_cali_chan_battery_handle = NULL;
static bool do_calibration1_chan_scale = false;
static bool do_calibration1_chan_battery = false;

// ADC Calibration
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

    if (!calibrated)
    {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK)
        {
            calibrated = true;
        }
    }

    *out_handle = handle;
    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated)
    {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else
    {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void adc_calibration_deinit(adc_cali_handle_t handle)
{
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));
}

static void adc_init(void)
{
    if (NULL == adc1_handle)
    {
        //-------------ADC1 Init---------------//
        adc_oneshot_unit_init_cfg_t init_config1 = {
            .unit_id = ADC_UNIT_1,
        };
        ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

        //-------------ADC1 Config---------------//
        adc_oneshot_chan_cfg_t config = {
            .bitwidth = ADC_BITWIDTH_DEFAULT,
            .atten = ADC_ATTEN,
        };
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN_SCALE, &config));
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC1_CHAN_BATTERY, &config));

        //-------------ADC1 Calibration Init---------------//
        do_calibration1_chan_scale = adc_calibration_init(ADC_UNIT_1, ADC1_CHAN_SCALE, ADC_ATTEN, &adc1_cali_chan_scale_handle);
        do_calibration1_chan_battery = adc_calibration_init(ADC_UNIT_1, ADC1_CHAN_BATTERY, ADC_ATTEN, &adc1_cali_chan_battery_handle);
    }
}

uint8_t get_battery_charge_in_perc(void)
{
    if (NULL == adc1_handle)
    {
        adc_init();
    }

    uint32_t voltage_sum = 0;
    for (size_t i = 0; i < ADC_VALUE_BUFFER_SIZE; i += 1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHAN_BATTERY, &adc_raw_battery[1]));
        if (do_calibration1_chan_battery)
        {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan_battery_handle, adc_raw_battery[1], &battery_voltage_in_mv[1]));
        }
    }
    uint32_t average_voltage = voltage_sum / ADC_VALUE_BUFFER_SIZE;

    // calculate percentage of battery charge here (depending on Hardware...)

    return 100; // placeholder
}

uint32_t get_weight_in_g(void)
{
    if (NULL == adc1_handle)
    {
        adc_init();
    }

    vTaskDelay(WAITING_TIME_BEFORE_ADC_READ_IN_S * 1000 / portTICK_PERIOD_MS);

    uint32_t voltage_sum = 0;
    for (size_t i = 0; i < ADC_VALUE_BUFFER_SIZE; i += 1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, ADC1_CHAN_SCALE, &adc_raw_scale[i]));
        if (do_calibration1_chan_scale)
        {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_chan_scale_handle, adc_raw_scale[i], &scale_voltage_in_mv[i]));
        }
        printf("scale_voltage_in_mv[%d]: %d\n", i, scale_voltage_in_mv[i]);
        voltage_sum += scale_voltage_in_mv[i];
    }
    uint32_t average_voltage = voltage_sum / ADC_VALUE_BUFFER_SIZE;

    float weight_in_g = LINEAR_OFFSET_IN_G - scale_constant_in_g_per_mv * average_voltage;

    // printf("average_voltage: %ld\n", average_voltage);
    // printf("Weight in g: %f\n", weight_in_g);
    // printf("int Weight in g: %ld\n", (uint32_t)weight_in_g);

    // return (uint32_t)weight_in_g;
    // return 1000; // placeholder value coz waage not wörking
    return average_voltage;
}
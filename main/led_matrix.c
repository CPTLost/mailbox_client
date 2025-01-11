/*******************************************************************
* File Name         : led_matrix.c
* Description       : Uses led_strip component to control a LED Matrix
*                    
* Author            : Noah Plank   
*
******************************************************************/

#include "led_matrix.h"

/// standard librarys
#include <stdio.h>
#include <esp_err.h>
#include <esp_log.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>

/// Components/other manually created header files
#include "led_strip.h"

#define GPIO_PIN_LED_MATRIX CONFIG_LED_MATRIX_GPIO
#define MATRIX_SIZE CONFIG_LED_MATRIX_SIZE

const char *TAG = "LED_MATRIX_DRIVER";

static bool g_led_matrix_configured = false;

static led_strip_handle_t led_matrix;

/// @brief initialize the LED Matrix
/// @param void
/// @return SUCCESS = 0, ALREADY_CONFIGURED = 1
return_val_t initLedMatrix(void)
{
    // It is not allowed to configure the same Matrix multiple times -> checks if g_led_matrix_configured == true
    if (false == g_led_matrix_configured)
    {
        g_led_matrix_configured = true;
        led_strip_config_t strip_config = {
            .strip_gpio_num = GPIO_PIN_LED_MATRIX,
            .max_leds = MATRIX_SIZE,
            .led_model = LED_MODEL_WS2812,
            .flags.invert_out = false,
        };
        led_strip_rmt_config_t rmt_config = {
            .resolution_hz = 10 * 1000 * 1000, // 10MHz
            .flags.with_dma = false,
        };
        ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_matrix));
        ESP_LOGI(TAG, "LED Matrix configured!");
        return SUCCESS;
    }
    else
    {
        ESP_LOGW(TAG, "LED Matrix ALREADY configured!");
        return ALREADY_CONFIGURED;
    }
}
/// @brief Set the color of the whole matrix
/// @param color 0 == RED, 1 == GREEN, 2 == BLUE these are defined in enum rgb_t
/// @param intensity
/// @return SUCCESS = 0
return_val_t led_matrix_set_color_hsv(rgb_t color, uint8_t intensity)
{
    for (size_t i = 0; i < MATRIX_SIZE; i += 1)
    {
        led_strip_set_pixel_hsv(led_matrix, i, 120 * color, 255, intensity);
    }
    led_strip_refresh(led_matrix);

    return SUCCESS;
}
/// @brief  Set the color of a specific pixel
/// @param color 0 == RED, 1 == GREEN, 2 == BLUE these are defined in enum rgb_t
/// @param intensity
/// @param pixel index of the pixel in the matrix
/// @return  SUCCESS = 0
return_val_t led_matrix_set_pixel_hsv(rgb_t color, uint8_t intensity, uint32_t pixel)
{
    if (pixel > (MATRIX_SIZE + 1))
    {
        ESP_LOGE(TAG, "Unvalid pixel index!");
        return UNVALID_VALUE;
    }
    led_strip_set_pixel_hsv(led_matrix, pixel, 120 * color, 255, intensity);

    led_strip_refresh(led_matrix);

    return SUCCESS;
}

return_val_t led_matrix_clear(void)
{
    led_strip_clear(led_matrix);
    return SUCCESS;
}

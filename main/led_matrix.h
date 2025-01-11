#ifndef LED_MATRIX_H
#define LED_MATRIX_H

#include "inttypes.h"
#include "return_val.h"

typedef enum
{
    RED,
    GREEN,
    BLUE,
} rgb_t;

return_val_t initLedMatrix(void);
return_val_t led_matrix_set_color_hsv(rgb_t, uint8_t);
return_val_t led_matrix_set_pixel_hsv(rgb_t, uint8_t, uint32_t);
return_val_t led_matrix_clear(void);

#endif

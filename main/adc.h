#ifndef ADC_H
#define ADC_H

#include "inttypes.h"

void adc_task(void *param);
uint8_t get_battery_charge_in_perc(void);
uint32_t get_weight_in_g(void);

#endif
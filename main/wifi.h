#ifndef WIFI_H
#define WIFI_H

#include "stdbool.h"
#include "return_val.h"

extern bool wifi_connected;

return_val_t wifi_init_sta(void);

#endif
#ifndef __TOUCH_H
#define __TOUCH_H

#include <soc/rmt_reg.h>
#include "driver/gpio.h" 
#include <esp_log.h>
#include "freertos/FreeRTOS.h"

#include "user_bsp_io_config.h"

#define TOUCH_DEBOUNCE_MS 50          // 去抖时间

esp_err_t touch_init(void);
bool touch_detected(void);

#endif
#ifndef __USER_BSP_H
#define __USER_BSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ble.h"
#include "led.h"
#include "nvs_flash.h"
#include "dc01.h"
#include "dht11.h"
#include "mq1_light.h"
#include "drv8833_fun.h"
#include "touch.h"

esp_err_t user_bsp_init(void);

#ifdef __cplusplus
}
#endif

#endif
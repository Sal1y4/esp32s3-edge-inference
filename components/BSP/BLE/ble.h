/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

 #ifndef _BLE_H
 #define _BLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_timer.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gatt_common_api.h"

#include "user_data.h"

typedef struct {
    uint16_t handle;
    uint16_t len;
    uint8_t value[ESP_GATT_MAX_ATTR_LEN]; // 固定缓冲区避免内存泄漏
} ble_data_t;

#define BLE_MIN(x, y) (((x) < (y)) ? (x) : (y))  // 避免与系统宏冲突

void ble_init(void);
esp_err_t ble_msg_send(uint8_t * value, uint16_t size);

#endif
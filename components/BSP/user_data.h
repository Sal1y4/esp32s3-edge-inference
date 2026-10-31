#ifndef __USER_DATA_H
#define __USER_DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"

typedef struct
{
    uint8_t frame_head;
    uint8_t car_state;
    uint8_t temp;
    uint8_t humi;
    uint8_t touch;
    uint8_t clean_prob;
    uint8_t resv;
    uint8_t msg_count;
    uint16_t mq1;
    uint16_t light;
    uint16_t dc01;
    uint16_t crc_16;
}USER_BLE_PROTOCOL_T;

typedef struct
{
    uint8_t frame_head;
    uint8_t car_state;
    uint8_t car_mode;
    uint16_t crc_16;
}USER_WX2CAR_T;

typedef struct
{
    float temp;
    float humi;
    float pm25;
    float aqi;
}MODEL_INPUT_DATA_T;

extern QueueHandle_t g_ble_recv_msg_queue;
extern USER_BLE_PROTOCOL_T g_sensor;

#endif
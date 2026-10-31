#ifndef __USER_TASK_H
#define __USER_TASK_H

#include "user_bsp.h"
#include "user_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SENSOR_PROC_TASK_STACK   4096
#define SENSOR_PROC_TASK_PRIO    5       // 高于BLE任务优先级
#define SENSOR_PROC_EVENT_QUEUE_LEN 5    // 事件队列深度

void task_sensor_proc_init(void);


#define GET_TOUCH_TASK_STACK   2048
#define GET_TOUCH_TASK_PRIO    6       // 高于BLE任务优先级
#define GET_TOUCH_EVENT_QUEUE_LEN 5    // 事件队列深度

void task_get_touch_init(void);


#define CAR_CONTROL_PROC_TASK_STACK 4096
#define CAR_CONTROL_PROC_TASK_PRIORITY 5

void task_car_fun_control_proc_init(void);

void user_task_init(void);

uint16_t crc16_modbus(uint8_t *data, uint16_t length);

#ifdef __cplusplus
}
#endif

void task_model_test_init(void);

#endif
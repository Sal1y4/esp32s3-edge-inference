#ifndef __DRV8833_FUN_H
#define __DRV8833_FUN_H

#include <soc/rmt_reg.h>
#include "driver/gpio.h" 
#include <esp_log.h>
#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "user_bsp_io_config.h"

typedef enum {
    MOTOR_A,
    MOTOR_B
} motor_channel_t;

typedef enum {
    MOTOR_STOP,
    MOTOR_FORWARD,
    MOTOR_BACKWARD
} motor_direction_t;


typedef struct {
    motor_direction_t motor_a;  // 电机A参数
    motor_direction_t motor_b;  // 电机B参数
} dual_motor_control_t;

#define FUN(x)          do { x ?                                      \
    gpio_set_level(FUN_GPIO_PIN, 1) :  \
    gpio_set_level(FUN_GPIO_PIN, 0); \
} while(0);  /* LED翻转 */

#define FUN_TOGGLE()    do { gpio_set_level(FUN_GPIO_PIN, !gpio_get_level(FUN_GPIO_PIN)); \
} while(0);  /* LED翻转 */

esp_err_t drv8833_fun_init(void);
extern void car_fun_control(uint8_t cmd);
extern void car_auto_clean(void);

#endif
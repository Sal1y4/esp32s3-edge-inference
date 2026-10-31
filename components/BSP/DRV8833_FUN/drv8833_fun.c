#include "drv8833_fun.h"

esp_err_t drv8833_fun_init(void)
{
    // 1. GPIO方向控制引脚初始化
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DRV8833_AIN1_GPIO_PIN) |
                       (1ULL << DRV8833_AIN2_GPIO_PIN) |
                       (1ULL << DRV8833_BIN1_GPIO_PIN) |
                       (1ULL << DRV8833_BIN2_GPIO_PIN) |
                       (1ull << FUN_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    FUN(0);                                                 /* 关闭风扇 */

    return ESP_OK;
}

static void drv8833_control(const dual_motor_control_t *ctrl)
{
    // 控制电机A方向
    switch(ctrl->motor_a)
    {
        case MOTOR_FORWARD:
            gpio_set_level(DRV8833_AIN1_GPIO_PIN, 1);
            gpio_set_level(DRV8833_AIN2_GPIO_PIN, 0);
        break;

        case MOTOR_BACKWARD:
            gpio_set_level(DRV8833_AIN1_GPIO_PIN, 0);
            gpio_set_level(DRV8833_AIN2_GPIO_PIN, 1);
        break;

        case MOTOR_STOP:
            gpio_set_level(DRV8833_AIN1_GPIO_PIN, 0);
            gpio_set_level(DRV8833_AIN2_GPIO_PIN, 0);
        break;
        
        default:
            gpio_set_level(DRV8833_AIN1_GPIO_PIN, 0);
            gpio_set_level(DRV8833_AIN2_GPIO_PIN, 0);
        break;
    }

    // 控制电机B方向
    switch(ctrl->motor_b)
    {
        case MOTOR_FORWARD:
            gpio_set_level(DRV8833_BIN1_GPIO_PIN, 1);
            gpio_set_level(DRV8833_BIN2_GPIO_PIN, 0);
        break;

        case MOTOR_BACKWARD:
            gpio_set_level(DRV8833_BIN1_GPIO_PIN, 0);
            gpio_set_level(DRV8833_BIN2_GPIO_PIN, 1);
        break;

        case MOTOR_STOP:
            gpio_set_level(DRV8833_BIN1_GPIO_PIN, 0);
            gpio_set_level(DRV8833_BIN2_GPIO_PIN, 0);
        break;
        
        default:
            gpio_set_level(DRV8833_BIN1_GPIO_PIN, 0);
            gpio_set_level(DRV8833_BIN2_GPIO_PIN, 0);
        break;
    }
}


void car_fun_control(uint8_t cmd)
{
    dual_motor_control_t car_ctrl;

    switch(cmd)
    {
        case 0:
            car_ctrl.motor_a = MOTOR_STOP;
            car_ctrl.motor_b = MOTOR_STOP;
            FUN(0);
            //ESP_LOGI("car_control", "停止");
        break;

        case 1:
            car_ctrl.motor_a = MOTOR_FORWARD;
            car_ctrl.motor_b = MOTOR_FORWARD;
            FUN(1);
            ESP_LOGI("car_control", "前进");
        break;

        case 2:
            car_ctrl.motor_a = MOTOR_BACKWARD;
            car_ctrl.motor_b = MOTOR_BACKWARD;
            FUN(1);
            ESP_LOGI("car_control", "后退");
        break;

        case 3:
            car_ctrl.motor_a = MOTOR_STOP;
            car_ctrl.motor_b = MOTOR_FORWARD;
            FUN(1);
            ESP_LOGI("car_control", "左转");
        break;

        case 4:
            car_ctrl.motor_a = MOTOR_FORWARD;
            car_ctrl.motor_b = MOTOR_STOP;
            FUN(1);
            ESP_LOGI("car_control", "右转");
        break;

        default:
            car_ctrl.motor_a = MOTOR_STOP;
            car_ctrl.motor_b = MOTOR_STOP;
            FUN(0);
            //ESP_LOGI("car_control", "停止");
        break;
    }

    drv8833_control(&car_ctrl);
}

void car_auto_clean(void)
{
    car_fun_control(1);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    car_fun_control(0);
    vTaskDelay(100 / portTICK_PERIOD_MS);

    car_fun_control(2);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    car_fun_control(0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
        
    car_fun_control(3);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    car_fun_control(0);
    vTaskDelay(100 / portTICK_PERIOD_MS);
        
    car_fun_control(4);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    car_fun_control(0);
}
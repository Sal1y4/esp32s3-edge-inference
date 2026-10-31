#ifndef __DHT11_H
#define __DHT11_H


#include "freertos/FreeRTOS.h"
#include <soc/rmt_reg.h>
#include "driver/gpio.h" 
#include <esp_log.h>

#include "user_bsp_io_config.h"

/* DHT11引脚高低电平枚举 */
typedef enum 
{
    DHT11_PIN_RESET = 0u,
    DHT11_PIN_SET
}DHT11_GPIO_PinState;

/* IO操作 */
#define DHT11_DQ_IN     gpio_get_level(DHT11_DQ_GPIO_PIN)   /* 数据端口输入 */

/* DHT11端口定义 */
#define DHT11_DQ_OUT(x) do{ x ?                                                 \
                            gpio_set_level(DHT11_DQ_GPIO_PIN, DHT11_PIN_SET) :  \
                            gpio_set_level(DHT11_DQ_GPIO_PIN, DHT11_PIN_RESET); \
                        }while(0)

/* 函数声明 */
void dht11_reset(void);                                 /* 复位DHT11 */
esp_err_t dht11_init(void);                               /* 初始化DHT11 */
esp_err_t dht11_check(void);                              /* 等待DHT11的回应 */
uint8_t get_dht11_data(uint8_t *temp,uint8_t *humi);   /* 读取温湿度 */

#endif
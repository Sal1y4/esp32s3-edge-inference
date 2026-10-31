#ifndef __USER_BSP_IO_CONFIG_H
#define __USER_BSP_IO_CONFIG_H

/* 引脚定义 */
#define LED_GPIO_PIN    GPIO_NUM_3  /* LED连接的GPIO端口 */
#define FUN_GPIO_PIN    GPIO_NUM_9  /* vcc:5v */

/* 引脚定义 */
#define DHT11_DQ_GPIO_PIN       GPIO_NUM_1  //vcc:3.3v

/* 引脚和串口定义 */
#define DC01_USART_1             UART_NUM_1  //vcc:5v
#define DC01_USART_TX_GPIO_PIN   GPIO_NUM_47
#define DC01_USART_RX_GPIO_PIN   GPIO_NUM_48


#define DRV8833_AIN1_GPIO_PIN    GPIO_NUM_10
#define DRV8833_AIN2_GPIO_PIN    GPIO_NUM_11
#define DRV8833_BIN1_GPIO_PIN    GPIO_NUM_12
#define DRV8833_BIN2_GPIO_PIN    GPIO_NUM_13

// ADC配置,vcc:
#define LIGHT_ADC_CHANNEL ADC_CHANNEL_5     // GPIO6
#define MQ1_ADC_CHANNEL   ADC_CHANNEL_6     // GPIO7

#define TOUCH_GPIO       GPIO_NUM_15

#endif
#ifndef __DC01_H
#define __DC01_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/uart_select.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "user_bsp_io_config.h"

/* 串口接收相关定义 */
#define RX_BUF_SIZE         1024    /* 环形缓冲区大小 */

extern esp_err_t dc01_init(void);
extern esp_err_t get_dc01_data(uint16_t *out_value);

#endif
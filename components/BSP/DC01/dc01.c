#include "dc01.h"

esp_err_t dc01_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT
    };
    uart_param_config(DC01_USART_1, &uart_config);          /* 配置uart端口 */

    /* 配置uart引脚 */
    uart_set_pin(DC01_USART_1, DC01_USART_TX_GPIO_PIN, DC01_USART_RX_GPIO_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    /* 安装串口驱动 */
    uart_driver_install(DC01_USART_1, RX_BUF_SIZE * 2, RX_BUF_SIZE * 2, 20, NULL, 0);
    
    return ESP_OK;
}

#define DC01_FRAME_SIZE 4
#define DC01_HEADER 0xA5

#define DC01_FRAME_SIZE 4
#define DC01_HEADER 0xA5

#define DC01_FRAME_SIZE      4
#define DC01_HEADER          0xA5
#define DC01_CHECKSUM_MASK   0x7F    // 校验和掩码
#define DATA_BITS_MASK       0x7F    // 数据位掩码


uint8_t rec_buff[RX_BUF_SIZE] = {0};

esp_err_t get_dc01_data(uint16_t *out_value)
{
    size_t len = 0;
    uint8_t data_high;
    uint8_t data_low;
    uint8_t calc_checksum;
    
    ESP_ERROR_CHECK(uart_get_buffered_data_len(DC01_USART_1, &len));
    //ESP_LOGI("data","uart pre len:%d", len);
    if (len != DC01_FRAME_SIZE) {
        uart_flush_input(DC01_USART_1);
        ESP_ERROR_CHECK(uart_get_buffered_data_len(DC01_USART_1, &len));
        //ESP_LOGI("data","uart flush len:%d", len);
        return ESP_ERR_INVALID_SIZE;
    }

    // 读取完整数据帧（带超时保护）
    len = uart_read_bytes(DC01_USART_1, rec_buff, DC01_FRAME_SIZE, pdMS_TO_TICKS(100));
    if (len != DC01_FRAME_SIZE) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // 协议特征字节校验
    if (rec_buff[0] != DC01_HEADER) {
        return ESP_ERR_INVALID_RESPONSE;
    }

    // 校验和验证（前3字节累加和低7位）
    calc_checksum = (rec_buff[0] + rec_buff[1] + rec_buff[2]) & DC01_CHECKSUM_MASK;
    if (rec_buff[3] != calc_checksum) {
        return ESP_ERR_INVALID_CRC;
    }

    // 数据解析（带位掩码处理）
    data_high = rec_buff[1] & DATA_BITS_MASK; // 高7位数据
    data_low  = rec_buff[2] & DATA_BITS_MASK; // 低7位数据
    
    // 浓度计算（符合文档公式：DATAH*128 + DATAL）
    *out_value = (data_high * 128) + data_low;
    *out_value /= 10;
    //ESP_LOGI("pm25", "%u", *out_value);
    
    // 限制out_value在0-200之间
    if (*out_value > 200) {
        *out_value = 200;
    }
    return ESP_OK;
}
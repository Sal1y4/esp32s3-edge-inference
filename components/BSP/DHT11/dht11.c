#include "dht11.h"


/**
 * @brief       复位DHT11
 * @param       data: 要写入的数据
 * @retval      无
 */
void dht11_reset(void)
{
    DHT11_DQ_OUT(0);        /* 拉低DQ */
    vTaskDelay(25);         /* 拉低至少18ms */
    DHT11_DQ_OUT(1);        /* DQ=1 */
    esp_rom_delay_us(30);   /* 主机拉高10~35us */
}

/**
 * @brief       等待DHT11的回应
 * @param       无
 * @retval      0, DHT11正常
 *              1, DHT11异常/不存在
 */
esp_err_t dht11_check(void)
{
    uint8_t retry = 0;
    esp_err_t rval = ESP_OK;

    while (DHT11_DQ_IN && retry < 100)      /* DHT11会拉低40~80us */
    {
        retry++;
        esp_rom_delay_us(1);
    }

    if (retry >= 100)
    {
        rval = ESP_FAIL;
    }
    else
    {
        retry = 0;

        while (!DHT11_DQ_IN && retry < 100) /* DHT11拉低后会再次拉高40~80us */
        {
            retry++;
            esp_rom_delay_us(1);
        }
        
        if (retry >= 100)
        {
            rval = ESP_FAIL;
        }
    }
    
    return rval;
}

/**
 * @brief       从DHT11读取一个位
 * @param       无
 * @retval      读取到的位值: 0 / 1
 */
uint8_t dht11_read_bit(void)
{
    uint8_t retry = 0;

    while (DHT11_DQ_IN && retry < 100)  /* 等待变为低电平 */
    {
        retry++;
        esp_rom_delay_us(1);
    }

    retry = 0;

    while (!DHT11_DQ_IN && retry < 100) /* 等待变高电平 */
    {
        retry++;
        esp_rom_delay_us(1);
    }

    esp_rom_delay_us(40);               /* 等待40us */

    if (DHT11_DQ_IN)                    /* 根据引脚状态返回 bit */
    {
        return 1;
    }
    else 
    {
        return 0;
    }
}

/**
 * @brief       从DHT11读取一个字节
 * @param       无
 * @retval      data：读到的数据
 */
static uint8_t dht11_read_byte(void)
{
    uint8_t i, data = 0;

    for (i = 0; i < 8; i++)         /* 循环读取8位数据 */
    {
        data <<= 1;                 /* 高位数据先输出, 先左移一位 */
        data |= dht11_read_bit();   /* 读取1bit数据 */
    }

    return data;
}

/**
 * @brief       从DHT11读取一次数据
 * @param       temp: 温度值(范围:-20~50°)
 * @param       humi: 湿度值(范围:5%~95%)
 * @retval      0, 正常.
 *              1, 失败
 */
uint8_t get_dht11_data(uint8_t *temp, uint8_t *humi)
{
    uint8_t buf[5];
    uint8_t i;

    dht11_reset();

    if (dht11_check() == ESP_OK)
    {
        for (i = 0; i < 5; i++) /* 读取40位数据 */
        {
            buf[i] = dht11_read_byte();
        }

        if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
        {
            *humi = buf[0];
            *temp = buf[2];

            // 限制湿度在45-75之间
            if (*humi < 45) {
                *humi = 45;
            } else if (*humi > 75) {
                *humi = 75;
            }

            // 限制温度在21-30之间
            if (*temp < 21) {
                *temp = 21;
            } else if (*temp > 30) {
                *temp = 30;
            }
        }
    }
    else
    {
        return 1;
    }
    
    return 0;
}

/**
 * @brief       初始化DHT11
 * @param       无
 * @retval      0, 正常
 *              1, 不存在/不正常
 */
esp_err_t dht11_init(void)
{
    gpio_config_t gpio_init_struct;

    gpio_init_struct.intr_type = GPIO_INTR_DISABLE;             /* 失能引脚中断 */
    gpio_init_struct.mode = GPIO_MODE_INPUT_OUTPUT_OD;          /* 开漏模式的输入和输出 */
    gpio_init_struct.pull_up_en = GPIO_PULLUP_ENABLE;           /* 使能上拉 */
    gpio_init_struct.pull_down_en = GPIO_PULLDOWN_DISABLE;      /* 失能下拉 */
    gpio_init_struct.pin_bit_mask = 1ull << DHT11_DQ_GPIO_PIN;  /* 设置的引脚的位掩码 */
    gpio_config(&gpio_init_struct);                             /* 配置DHT11引脚 */

    dht11_reset();
    return dht11_check();
}


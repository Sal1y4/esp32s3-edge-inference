#include "user_bsp.h"

esp_err_t user_bsp_init(void)
{
    esp_err_t ret; // 错误状态存储变量

    /* 初始化非易失性存储(NVS) */
    ret = nvs_flash_init();
    // 处理NVS初始化错误（无空闲页或发现新版本）
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); // 擦除NVS分区
        ret = nvs_flash_init();              // 重新初始化
    }
    ESP_ERROR_CHECK( ret ); // 检查最终初始化结果

    led_init();
    ble_init();
    dc01_init();
    dht11_init();
    mq1_light_init();
    drv8833_fun_init();
    return ESP_OK;
}
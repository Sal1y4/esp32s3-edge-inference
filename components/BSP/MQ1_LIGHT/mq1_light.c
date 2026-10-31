#include "mq1_light.h"
#include <math.h>

#define VCC        5.0f
#define ADC_MAX    3300.0f // 电压最大值（mV）
#define RL         10000.0f
#define RO         10000.0f // 你可以在空气中通过实验确定Ro
#define A_CONST    100.0f
#define B_CONST    1.5f

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle;


// 统一初始化函数
void mq1_light_init(void)
{
    // ADC单元初始化
    adc_oneshot_unit_init_cfg_t unit_cfg = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc_handle));

    // 校准配置
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &cali_handle));

    // 双通道配置
    adc_oneshot_chan_cfg_t ch_cfg = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12
    };
    
    // 配置MQ1通道
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle, 
        MQ1_ADC_CHANNEL, 
        &ch_cfg
    ));

    // 配置光敏通道
    ESP_ERROR_CHECK(adc_oneshot_config_channel(
        adc_handle,
        LIGHT_ADC_CHANNEL,
        &ch_cfg
    ));
}

// 统一数据获取函数
void get_mq1_light_data(uint16_t *mq1_data, uint16_t *light_data)
{
    int raw = 0;
    int voltage = 0;
    
    /* 读取MQ1数据 */
    if(adc_oneshot_read(adc_handle, MQ1_ADC_CHANNEL, &raw) == ESP_OK) {
        if(adc_cali_raw_to_voltage(cali_handle, raw, &voltage) == ESP_OK) {
            //ESP_LOGI("AQI", "%u", voltage);
            *mq1_data = (voltage / 200)*16;
        }
    }
    
    /* 读取光敏数据 */
    if(adc_oneshot_read(adc_handle, LIGHT_ADC_CHANNEL, &raw) == ESP_OK) {
        if(adc_cali_raw_to_voltage(cali_handle, raw, &voltage) == ESP_OK) {
            *light_data = 3300 - voltage;
        }
    }
}
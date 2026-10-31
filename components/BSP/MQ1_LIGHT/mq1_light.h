#ifndef __MQ1_LIGHT_H
#define __MQ1_LIGHT_H

#include <soc/rmt_reg.h>
#include "driver/gpio.h" 
#include <esp_log.h>
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali_scheme.h"

#include "user_bsp_io_config.h"

void mq1_light_init(void);
void get_mq1_light_data(uint16_t *mq1_data, uint16_t *light_data);

#endif
#ifndef __MODEL_HPP
#define __MODEL_HPP

#include <soc/rmt_reg.h>
#include "driver/gpio.h" 
#include <esp_log.h>
#include "user_data.h"

#define BATCH_SIZE  1
#define TIME_STEPS  12
#define FEATURE_NUM 6

#define FEATURE_MIN_TEMP   19.95999913f
#define FEATURE_MIN_HUMI   43.30737291f
#define FEATURE_MIN_PM25   19.24016698f
#define FEATURE_MIN_AQI    15.17155725f

#define FEATURE_RANGETEMP  10.01542005f
#define FEATURE_RANGEHUMI  34.49400546f
#define FEATURE_RANGEPM25  124.10405182f
#define FEATURE_RANGEAQI   167.21883223f

//pm25 > 90,  aqi > 110代表需要清洁

extern void model_init(void);
extern void model_sim_test(void);
extern void model_proc(USER_BLE_PROTOCOL_T *p_sensor_data);
extern void model_deinit(void);
#endif
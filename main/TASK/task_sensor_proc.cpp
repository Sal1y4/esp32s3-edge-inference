#include "user_task.h"
#include "model.hpp"

static uint8_t run_count;
static uint32_t model_run_count;

char test_data[4] = {0xA5, 0x01, 0x2C, 0x52};

static void task_proc_func(void *arg) 
{
    
    esp_err_t ret;
    g_sensor.frame_head = 0xf5;
    ESP_LOGI("task_sensor_proc", "Task started");
    model_init();
    
    while(1) 
    {
        get_dc01_data(&g_sensor.dc01);

        run_count++;
        if(0 == (run_count % 5))
        {
            get_dht11_data(&g_sensor.temp, &g_sensor.humi);
            get_mq1_light_data(&g_sensor.mq1, &g_sensor.light);
            model_proc(&g_sensor);
            ESP_LOGW("task_sensor_proc", "%lu", model_run_count++);
            g_sensor.crc_16 = crc16_modbus((uint8_t *)&g_sensor.car_state, sizeof(USER_BLE_PROTOCOL_T) - 3);
            //ESP_LOG_BUFFER_HEXDUMP("task_sensor_proc", (uint8_t *)&g_sensor, sizeof(USER_BLE_PROTOCOL_T), ESP_LOG_INFO);
            ret = ble_msg_send((uint8_t *)&g_sensor, sizeof(USER_BLE_PROTOCOL_T));
            if(ESP_OK == ret)
            {
                g_sensor.msg_count++;
            }
        }

        vTaskDelay(500 / portTICK_PERIOD_MS);
    }

    model_deinit();
    vTaskDelete(NULL);
}

void task_sensor_proc_init(void)
{
    xTaskCreatePinnedToCore(
        task_proc_func,
        "task_sensor_proc",
        SENSOR_PROC_TASK_STACK,
        NULL,
        SENSOR_PROC_TASK_PRIO,
        NULL,
        APP_CPU_NUM
    );
}
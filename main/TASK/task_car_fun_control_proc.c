#include "user_task.h"

static ble_data_t s_rx_data;

static USER_WX2CAR_T s_car_cmd;

static void task_proc_func(void *arg) 
{
    ESP_LOGI("task_ble_proc", "Task started");

    while(1)
    {
        if (xQueueReceive(g_ble_recv_msg_queue, &s_rx_data, 0)) 
        {
            ESP_LOGI("BLE", "特征句柄[0x%04X] 数据长度:%d", s_rx_data.handle, s_rx_data.len);
            ESP_LOG_BUFFER_HEXDUMP("BLE", s_rx_data.value, s_rx_data.len, ESP_LOG_INFO);
            memcpy(&s_car_cmd, s_rx_data.value, s_rx_data.len);
        }

        if(s_car_cmd.car_mode == 0x00)
        {
            car_fun_control(s_car_cmd.car_state);
        }
        else if(s_car_cmd.car_mode == 0x01)
        {
            if(g_sensor.clean_prob > 50)
            {
                car_auto_clean();
            }
        }
        else{
            ;
        }
        
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void task_car_fun_control_proc_init(void)
{
    xTaskCreatePinnedToCore(
        task_proc_func,
        "car_control_proc",
        CAR_CONTROL_PROC_TASK_STACK,
        NULL,
        CAR_CONTROL_PROC_TASK_PRIORITY,
        NULL,
        APP_CPU_NUM
    );
}
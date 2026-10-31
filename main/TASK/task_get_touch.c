#include "user_task.h"

static void task_proc_func(void *arg) 
{
    ESP_LOGI("task_get_touch", "Task started");
    
    while(1) 
    {
        if(touch_detected()) 
        {

            
            ESP_LOGD("TOUCH", "Event generated");
        }
        vTaskDelay(pdMS_TO_TICKS(20)); // 20ms检测周期
    }
}

void task_get_touch_init(void)
{
    xTaskCreatePinnedToCore(
        task_proc_func,
        "get_touch_data",
        GET_TOUCH_TASK_STACK,
        NULL,
        GET_TOUCH_TASK_PRIO,
        NULL,
        APP_CPU_NUM
    );
}
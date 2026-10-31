#include "user_task.h"

void app_main(void)
{
    user_bsp_init();
    user_task_init();

    while(1)
    {
        LED_TOGGLE();
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
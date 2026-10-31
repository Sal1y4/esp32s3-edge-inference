#include "touch.h"

esp_err_t touch_init(void)
{
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << TOUCH_GPIO),
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,    // 触摸传感器需要上拉（网页22建议）
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    return gpio_config(&io_conf);
}

// 增强型触摸检测（带状态机）
bool touch_detected(void) 
{
    static enum { IDLE, PRESSED, RELEASED } state = IDLE;
    static TickType_t last_change = 0;
    
    bool current_state = (gpio_get_level(TOUCH_GPIO) == 0);
    TickType_t now = xTaskGetTickCount();
    
    switch(state) {
    case IDLE:
        if(current_state) {
            state = PRESSED;
            last_change = now;
        }
        break;
        
    case PRESSED:
        if((now - last_change) > pdMS_TO_TICKS(TOUCH_DEBOUNCE_MS)) {
            if(!current_state) {
                state = IDLE;
            } else {
                state = RELEASED;
                return true;  // 有效触发
            }
        }
        break;
        
    case RELEASED:
        if(!current_state) {
            state = IDLE;
        }
        break;
    }
    
    return false;
}
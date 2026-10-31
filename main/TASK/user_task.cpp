#include "user_task.h"

// x16 + x15 + x2 + 1
// CRC-16/DNP 多项式 0x8005, LSB优先，0x8005 反射后为 0xA001
// 0x8005 1000 0000 0000 0101
// 0xA001 1010 0000 0000 0001
uint16_t crc16_modbus(uint8_t *data, uint16_t length) 
{
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

void user_task_init(void)
{
    task_get_touch_init();
    task_sensor_proc_init();
    task_car_fun_control_proc_init();
    //task_model_test_init(); //FIXME:模型测试任务，工程化不需要开启
}
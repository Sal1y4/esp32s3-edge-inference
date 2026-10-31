#include "model.hpp"
#include "dl_model_base.hpp"
#include "fbs_loader.hpp"

typedef struct EspModelContext {
    dl::Model* model = nullptr;
    dl::TensorBase* input_tensor = nullptr;
    dl::TensorBase* output_tensor = nullptr;
    std::string input_name;
    std::string output_name;
    std::map<std::string, dl::TensorBase*> input_map;
    std::map<std::string, dl::TensorBase*> output_map;
} ESP_MODEL_CONTEXT_T;

static ESP_MODEL_CONTEXT_T *s_esp_model = nullptr;
static MODEL_INPUT_DATA_T s_model_input_new_data;

static MODEL_INPUT_DATA_T s_model_sim_input_data[TIME_STEPS * 3] = {
    {25.496f, 72.778f, 100.99f, 155.567f},  // 第0小时
    {24.937f, 75.725f, 94.624f, 128.963f},  // 第1小时
    {25.798f, 65.791f, 82.404f, 123.585f},  // 第2小时
    {26.749f, 71.660f, 72.560f, 139.818f},  // 第3小时
    {25.067f, 67.997f, 93.499f, 136.393f},  // 第4小时
    {25.142f, 68.459f, 89.669f, 130.618f},  // 第5小时
    {27.030f, 68.108f, 97.943f, 153.472f},  // 第6小时
    {26.293f, 67.253f, 94.788f, 158.486f},  // 第7小时
    {25.130f, 69.943f, 101.74f, 140.959f},  // 第8小时
    {26.216f, 67.251f, 78.718f, 168.527f},  // 第9小时
    {25.284f, 70.495f, 107.23f, 115.904f},  // 第10小时
    {25.354f, 69.468f, 91.168f, 134.601f},   // 第11小时

    {23.861f, 50.414f, 63.423f, 43.360f},  // 第12小时
    {24.317f, 50.454f, 65.589f, 73.655f},  // 第13小时
    {22.233f, 43.307f, 48.257f, 61.849f},  // 第14小时
    {22.085f, 51.366f, 42.455f, 85.831f},  // 第15小时
    {23.864f, 50.972f, 87.338f, 90.410f},  // 第16小时
    {23.576f, 53.896f, 50.092f, 119.02f},  // 第17小时,T5 的 AQI 已超过阈值（119），模型学到这类环境模式很可能下一小时需要清洁。
    {23.468f, 57.501f, 47.312f, 79.410f},  // 第18小时
    {23.504f, 55.486f, 29.280f, 74.942f},  // 第19小时
    {22.419f, 51.456f, 51.406f, 72.942f},  // 第20小时
    {23.273f, 49.106f, 40.566f, 62.639f},  // 第21小时
    {23.277f, 60.312f, 84.476f, 77.139f},  // 第22小时
    {22.214f, 52.941f, 74.440f, 65.147f},   // 第23小时

    {21.0f, 45.0f, 10.0f, 20.0f},  // 低PM2.5、低AQI
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f},
    {21.0f, 45.0f, 10.0f, 20.0f}
};

static MODEL_INPUT_DATA_T s_model_input_data[TIME_STEPS];

// 修正后的24小时时间编码特征值（基于标准三角函数计算）
static float day_sin_values[24] = {
    /* 0-11时 */
    0.0000, 0.2588, 0.5000, 0.7071, 0.8660, 0.9659, // 0-5时
    1.0000, 0.9659, 0.8660, 0.7071, 0.5000, 0.2588, // 6-11时
    
    /* 12-23时 */
    0.0000, -0.2588, -0.5000, -0.7071, -0.8660, -0.9659, // 12-17时
    -1.0000, -0.9659, -0.8660, -0.7071, -0.5000, -0.2588  // 18-23时
};

static float day_cos_values[24] = {
    /* 0-11时 */
    1.0000, 0.9659, 0.8660, 0.7071, 0.5000, 0.2588, // 0-5时
    0.0000, -0.2588, -0.5000, -0.7071, -0.8660, -0.9659, // 6-11时
    
    /* 12-23时 */
    -1.0000, -0.9659, -0.8660, -0.7071, -0.5000, -0.2588, // 12-17时
    0.0000, 0.2588, 0.5000, 0.7071, 0.8660, 0.9659  // 18-23时
};

void model_init(void) 
{
    // === 1. 模型加载与初始化 ===
    if (!s_esp_model) {
        s_esp_model = new ESP_MODEL_CONTEXT_T();
    }
    s_esp_model->model = new dl::Model("model", fbs::MODEL_LOCATION_IN_FLASH_PARTITION);
    if (!s_esp_model->model) {
        ESP_LOGE("Model", "Context not initialized");
        return;
    }

    // 打印模型输入名称与形状（调试用）
    for (const auto& pair : s_esp_model->model->get_inputs()) {
        ESP_LOGI("Model 输入", "Input name: %s", pair.first.c_str());
        auto shape = pair.second->shape;
        for (auto dim : shape) {
            ESP_LOGI("Model Input", " - shape dim: %d", dim);
        }
    }

    //s_esp_model->model->profile_memory();
    //s_esp_model->model->profile_module(true);

    // 创建输入张量
    s_esp_model->input_tensor = new dl::TensorBase(
        {BATCH_SIZE, TIME_STEPS, FEATURE_NUM},
        calloc(1, sizeof(float) * BATCH_SIZE * TIME_STEPS * FEATURE_NUM), // 使用calloc初始化
        0,
        dl::DATA_TYPE_FLOAT  
    );

    // 创建输出张量
    s_esp_model->output_tensor = new dl::TensorBase(
        {BATCH_SIZE, 1},
        calloc(1, sizeof(float) * BATCH_SIZE * 1), // 使用calloc初始化
        0,
        dl::DATA_TYPE_FLOAT
    );
    // 获取输入输出名
    s_esp_model->input_name = s_esp_model->model->get_inputs().begin()->first;
    s_esp_model->output_name = s_esp_model->model->get_outputs().begin()->first;

    s_esp_model->input_map = {{s_esp_model->input_name, s_esp_model->input_tensor}};
    s_esp_model->output_map = {{s_esp_model->output_name, s_esp_model->output_tensor}};
}

void model_input_data_pre_proc(MODEL_INPUT_DATA_T data[TIME_STEPS], uint8_t time)
{
    // 创建临时浮点张量（类似 run3 方式）
    dl::TensorBase *input_float = new dl::TensorBase(
        {BATCH_SIZE, TIME_STEPS, FEATURE_NUM},
        calloc(BATCH_SIZE * TIME_STEPS * FEATURE_NUM, sizeof(float)), // 分配内存
        0,
        dl::DATA_TYPE_FLOAT
    );

    float *input_ptr = (float *)input_float->data;
    for (int b = 0; b < BATCH_SIZE; ++b) 
    {
        for (int t = 0; t < TIME_STEPS; ++t) 
        {
            // 原始数据填充（保持浮点精度）
            input_ptr[(b*TIME_STEPS + t)*FEATURE_NUM + 0] = (data[t].temp - FEATURE_MIN_TEMP) / FEATURE_RANGETEMP;
            input_ptr[(b*TIME_STEPS + t)*FEATURE_NUM + 1] = (data[t].humi - FEATURE_MIN_HUMI) / FEATURE_RANGEHUMI;
            input_ptr[(b*TIME_STEPS + t)*FEATURE_NUM + 2] = (data[t].pm25 - FEATURE_MIN_PM25) / FEATURE_RANGEPM25;
            input_ptr[(b*TIME_STEPS + t)*FEATURE_NUM + 3] = (data[t].aqi  - FEATURE_MIN_AQI ) / FEATURE_RANGEAQI ;
            
            // 时间编码直接存储原始值（自动量化）
            input_ptr[(b*TIME_STEPS + t)*FEATURE_NUM + 4] = fmaxf(fminf(day_sin_values[t + time], 1.0f), -1.0f);
            input_ptr[(b*TIME_STEPS + t)*FEATURE_NUM + 5] = fmaxf(fminf(day_cos_values[t + time], 1.0f), -1.0f);
        }
    }

    s_esp_model->input_tensor->assign(input_float);

    delete input_float; // 及时释放临时张量
}

void model_run(void)
{
    if (!s_esp_model || 
        !s_esp_model->model || 
        !s_esp_model->input_tensor || 
        !s_esp_model->output_tensor) 
    {
        ESP_LOGE("Model", "Model components not initialized");
        return;
    }
    
    // 确保map包含有效指针
    if (s_esp_model->input_map.empty() || 
        s_esp_model->output_map.empty())
    {
        ESP_LOGE("Model", "Input/Output map not configured");
        return;
    }
    
    s_esp_model->model->run(
        s_esp_model->input_map,
        dl::RUNTIME_MODE_MULTI_CORE,
        s_esp_model->output_map
    );
}

float model_output_data_post_proc(void)
{
    dl::TensorBase *output_float = new dl::TensorBase(
        {BATCH_SIZE, 1}, 
        calloc(BATCH_SIZE*1, sizeof(float)),
        0,
        dl::DATA_TYPE_FLOAT
    );
    
    output_float->assign(s_esp_model->output_tensor);
    float clean_prob = ((float*)output_float->data)[0]; // 删除Sigmoid

    delete output_float;
    return clean_prob;
}

void model_deinit(void)
{
    delete s_esp_model->input_tensor;
    delete s_esp_model->output_tensor;
    delete s_esp_model->model;
    s_esp_model = nullptr;
}


void model_sim_test(void)
{
    static uint8_t window_index = 0;
    static uint8_t time = 0;
    int64_t start_time, end_time;
    float clean_prob = 0;

    start_time = esp_timer_get_time();
    // 将旧数据向前滑动，丢弃最早的一个
    memmove(&s_model_input_data[0],             // 目标起始地址
            &s_model_input_data[1],             // 源起始地址（从第2个数据开始）
            sizeof(MODEL_INPUT_DATA_T) * (TIME_STEPS - 1)); // 复制长度（向前滑动一格）
    // 插入新数据到最后一位
    memcpy(&s_model_input_data[TIME_STEPS - 1],
           &s_model_sim_input_data[window_index],
           sizeof(MODEL_INPUT_DATA_T));
    model_input_data_pre_proc(s_model_input_data, time);
    model_run();
    clean_prob = model_output_data_post_proc(); //I (89682) Output: 原始量化值: 0 → 反量化值: 0.9375 → 决策: 清洁
    end_time = esp_timer_get_time();
    ESP_LOGI("model_sim_test", "window_index %d, time: %lld us, 预测未来1小时清扫概率: %.4f → 决策: %s", 
                               window_index, end_time - start_time, clean_prob, clean_prob > 0.5f ? "清洁" : "等待");

    if(time > (TIME_STEPS - 1))
    {
        time = 0;
    }
    else {
        time++;
    }

    if(window_index > ((TIME_STEPS * 3) - 2))
    {
        window_index = 0;
    }
    else {
        window_index++;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void model_proc(USER_BLE_PROTOCOL_T *p_sensor_data)
{
    static uint8_t time = 0;
    int64_t start_time, end_time;
    float post_data = 0;
    
    s_model_input_new_data.temp = p_sensor_data->temp;
    s_model_input_new_data.humi = p_sensor_data->humi;
    s_model_input_new_data.pm25 = p_sensor_data->dc01;
    s_model_input_new_data.aqi = p_sensor_data->mq1;

    start_time = esp_timer_get_time();
    // 将旧数据向前滑动，丢弃最早的一个
    memmove(&s_model_input_data[0],             // 目标起始地址
            &s_model_input_data[1],             // 源起始地址（从第2个数据开始）
            sizeof(MODEL_INPUT_DATA_T) * (TIME_STEPS - 1)); // 复制长度（向前滑动一格）
    // 插入新数据到最后一位
    memcpy(&s_model_input_data[TIME_STEPS - 1],
           &s_model_input_new_data,
           sizeof(MODEL_INPUT_DATA_T));
    model_input_data_pre_proc(s_model_input_data, time);
    model_run();
    post_data = model_output_data_post_proc(); //I (89682) Output: 原始量化值: 0 → 反量化值: 0.9375 → 决策: 清洁
    p_sensor_data->clean_prob = post_data*100;
    end_time = esp_timer_get_time();
    ESP_LOGI("task_sensor_proc", 
        "温度:%u°C, 湿度:%u%%, PM25:%u, AQI:%u, 光强:%u, 清扫概率:%d%% → 决策: %s, 模型推理耗时:%lld us, msg_count: %u",
        p_sensor_data->temp, 
        p_sensor_data->humi, 
        p_sensor_data->dc01,
        p_sensor_data->mq1, 
        p_sensor_data->light,
        p_sensor_data->clean_prob,
        post_data > 0.5f ? "清洁" : "等待",
        end_time - start_time,
        p_sensor_data->msg_count);

    if(time > (TIME_STEPS - 1))
    {
        time = 0;
    }
    else {
        time++;
    }
}
#include "user_task.h"
#include "model.hpp"

#if 0
void run1()
{
    dl::Model *model = new dl::Model("model", fbs::MODEL_LOCATION_IN_FLASH_PARTITION); //从 partition 中加载模型

    //ESP_ERROR_CHECK(model->test()); //测试模型板端推理是否正确
    model->profile_memory(); //测试模型内存占用
    model->profile_module(); //测试模型推理耗时,默认按照 ONNX 拓扑排序打印模型各层。如果想按照各层的耗时来排序，可以将 profile_module 的入参设为 True,  model->profile_module(true);

    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs(); //model->get_inputs() 返回一个 map（字典），key 是输入名字，value 是对应的输入张量。
    dl::TensorBase *model_input = model_inputs.begin()->second;                  //这里只取第一个输入张量，并用指针 model_input 指向它。
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();//model->get_outputs() 返回一个 map（字典），key 是输入名字，value 是对应的输出张量。
    dl::TensorBase *model_output = model_outputs.begin()->second;                //这里只取第一个输出张量，并用指针 model_output 指向它。

    // quantize the float input and feed it into model.
    float input_v = degree * M_PI / 180;
    // Note that dl::quantize accepts inverse of scale as the second input, so we use DL_RESCALE here.
    int8_t quant_input_v = dl::quantize<int8_t>(input_v, DL_RESCALE(model_input->exponent)); //把 float 输入转换为 int8_t 类型
    // assign quant_input_v to input.
    int8_t *input_ptr = (int8_t *)model_input->data;
    *input_ptr = quant_input_v;   //把量化后的值写入模型输入张量内存
    // print input info and value.
    model_input->print(true);

    model->run();

    // get the model output and dequantize it into float. Use pointer to do this element by element;
    // model_output->print(true);
    int8_t *output_ptr = (int8_t *)model_output->data;
    int8_t quant_output_v = *output_ptr; //获取输出张量的数据地址，取出第一个值（假设输出 shape 是 [1,1]）
    float output_v = dl::dequantize(quant_output_v, DL_SCALE(model_output->exponent));  //将 int8_t 类型的输出值反量化还原为 float

    printf("sin(%d degree) = %f\n", degree, output_v);

    delete model; //释放模型对象，避免内存泄漏。
}

// same as run1, different way to quantize input & dequantize output
void run2()
{
    dl::Model *model = new dl::Model("model", fbs::MODEL_LOCATION_IN_FLASH_PARTITION); //从 partition 中加载模型

    //ESP_ERROR_CHECK(model->test()); //测试模型板端推理是否正确
    model->profile_memory(); //测试模型内存占用
    model->profile_module(); //测试模型推理耗时,默认按照 ONNX 拓扑排序打印模型各层。如果想按照各层的耗时来排序，可以将 profile_module 的入参设为 True,  model->profile_module(true);

    std::map<std::string, dl::TensorBase *> model_inputs = model->get_inputs();
    dl::TensorBase *model_input = model_inputs.begin()->second;
    std::map<std::string, dl::TensorBase *> model_outputs = model->get_outputs();
    dl::TensorBase *model_output = model_outputs.begin()->second;

    // quantize the float input and feed it into model.
    float input_v = degree * M_PI / 180;
    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *input_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT); //创建一个 shape 为 [1, 1] 的浮点张量，初始内容为 0。
    float *input_tensor_ptr = (float *)input_tensor->data;
    *input_tensor_ptr = input_v;     //把计算出的弧度值 input_v 写入到输入张量中。
    model_input->assign(input_tensor); //将 input_tensor 中的浮点值赋值到模型实际使用的 model_input 中。⚠️ 此函数会自动完成量化，按模型的量化参数处理。

    model->run();

    // get the model output and dequantize it into float.
    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *output_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT); //创建一个 shape 为 [1, 1] 的浮点输出张量，用于保存反量化后的输出。
    output_tensor->assign(model_output);                      //将模型输出（量化数据）赋值给 output_tensor，同时会自动反量化成 float 类型。
    float *output_tensor_ptr = (float *)output_tensor->data;
    float output_v = *output_tensor_ptr;   //取出反量化后的浮点结果。

    printf("sin(%d degree) = %f\n", degree, output_v);

    delete input_tensor;
    delete output_tensor;
    delete model;
}

// shows how to use model->run(std::map<std::string, TensorBase *> &user_inputs, runtime_mode_t mode,
// std::map<std::string, TensorBase *> user_outputs) api
void run3()
{
    dl::Model *model = new dl::Model("model", fbs::MODEL_LOCATION_IN_FLASH_PARTITION); //从 partition 中加载模型

    //ESP_ERROR_CHECK(model->test()); //测试模型板端推理是否正确
    model->profile_memory(); //测试模型内存占用
    model->profile_module(); //测试模型推理耗时,默认按照 ONNX 拓扑排序打印模型各层。如果想按照各层的耗时来排序，可以将 profile_module 的入参设为 True,  model->profile_module(true);

    // quantize the float input and feed it into model.
    float input_v = degree * M_PI / 180;
    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *input_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT);//创建一个大小为 [1, 1] 的 float 类型张量 input_tensor，默认值为 0。
    float *input_tensor_ptr = (float *)input_tensor->data;
    *input_tensor_ptr = input_v;  //将计算出来的弧度值 input_v 写入 input_tensor 中
    std::string input_name = model->get_inputs().begin()->first;  //获取模型第一个输入张量的名字。
    std::map<std::string, dl::TensorBase *> input_map = {{input_name, input_tensor}}; //创建一个 std::map，把 input_tensor 作为指定输入名称的输入传入模型。

    // create a TensorBase filled with 0 of shape [1, 1]
    dl::TensorBase *output_tensor = new dl::TensorBase({1, 1}, nullptr, 0, dl::DATA_TYPE_FLOAT); //创建一个空的 float 类型的输出张量 output_tensor，shape 为 [1, 1]
    std::string output_name = model->get_outputs().begin()->first; //获取模型第一个输出张量的名字。
    std::map<std::string, dl::TensorBase *> output_map = {{output_name, output_tensor}}; //创建一个 std::map，将 output_tensor 与输出名绑定，用于接收推理输出。

    model->run(input_map, dl::RUNTIME_MODE_SINGLE_CORE, output_map); //使用手动指定的输入输出映射执行模型推理,dl::RUNTIME_MODE_SINGLE_CORE：模型只使用一个核心运行，适用于资源受限的情况。

    float *output_tensor_ptr = (float *)output_tensor->data;
    float output_v = *output_tensor_ptr;   //从 output_tensor 中读取模型的推理输出值（反量化后的 float 值）

    printf("sin(%d degree) = %f\n", degree, output_v);

    delete input_tensor;
    delete output_tensor;
    delete model;
}

#endif
static void task_proc_func(void *arg) 
{
    ESP_LOGI("task_model_proc", "Task started");
    model_init();

    while(1)
    {
        model_sim_test();
    }
    model_deinit();
    vTaskDelete(NULL);
}

void task_model_test_init(void)
{
    xTaskCreatePinnedToCore(
        task_proc_func,
        "model_test",
        4096,
        NULL,
        5,
        NULL,
        APP_CPU_NUM
    );
}
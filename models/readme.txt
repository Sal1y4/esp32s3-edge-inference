# 环境要求

## Python 版本
- Python 3.9.21 或更高版本

## 主要依赖库
conda env list
conda activate esp_model
pip list
pip list > conda_esp_model_python3_9_21_requirements.txt
pip list > my_conda_esp_model_python3_9_21_requirements.txt

conda_esp_model_python3_9_21_requirements.txt

#   ESP32-DL库支持的算子
#    this->register_module("Conv", Conv2D::deserialize);
#    this->register_module("Add", Add::deserialize);
#    this->register_module("Sub", Sub::deserialize);
#    this->register_module("Mul", Mul::deserialize);
#    this->register_module("Div", Div::deserialize);
#    this->register_module("Resize", Resize2D::deserialize);
#    this->register_module("GlobalAveragePool", GlobalAveragePool2D::deserialize);
#    this->register_module("AveragePool", AveragePool2D::deserialize);
#    this->register_module("Concat", Concat::deserialize);
#    this->register_module("Sigmoid", Sigmoid::deserialize);
#    this->register_module("Tanh", Tanh::deserialize);
#    this->register_module("Relu", Relu::deserialize);
#    this->register_module("LeakyRelu", LeakyRelu::deserialize);
#    this->register_module("HardSigmoid", HardSigmoid::deserialize);
#    this->register_module("HardSwish", HardSwish::deserialize);
#    this->register_module("Gelu", LUT::deserialize);
#    this->register_module("LUT", LUT::deserialize);
#    this->register_module("Gemm", Gemm::deserialize);
#    this->register_module("QuantizeLinear", RequantizeLinear::deserialize);
#    this->register_module("DequantizeLinear", RequantizeLinear::deserialize);
#    this->register_module("RequantizeLinear", RequantizeLinear::deserialize);
#    this->register_module("PRelu", PRelu::deserialize);
#    this->register_module("Clip", Clip::deserialize);
#    this->register_module("Flatten", Flatten::deserialize);
#    this->register_module("Reshape", Reshape::deserialize);
#    this->register_module("Transpose", Transpose::deserialize);
#    this->register_module("Exp", Exp::deserialize);
#    this->register_module("Log", Log::deserialize);
#    this->register_module("Sqrt", Sqrt::deserialize);
#    this->register_module("Squeeze", Squeeze::deserialize);
#    this->register_module("Unsqueeze", Unsqueeze::deserialize);
#    this->register_module("Softmax", Softmax::deserialize);
#    this->register_module("MaxPool", MaxPool2D::deserialize);
#    this->register_module("Slice", Slice::deserialize);
#    this->register_module("Pad", Pad::deserialize);
#    this->register_module("MatMul", MatMul::deserialize);
#    this->register_module("Split", Split::deserialize);
#    this->register_module("Gather", Gather::deserialize);
#    this->register_module("Elu", Elu::deserialize);
#    this->register_module("Greater", Greater::deserialize);
#    this->register_module("GreaterOrEqual", GreaterOrEqual::deserialize);
#    this->register_module("Equal", Equal::deserialize);
#    this->register_module("Less", Less::deserialize);
#    this->register_module("LessOrEqual", LessOrEqual::deserialize);
#    this->register_module("ReverseSequence", ReverseSequence::deserialize);
#    this->register_module("Identity", Identity::deserialize);
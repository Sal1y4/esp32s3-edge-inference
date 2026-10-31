import numpy as np
import onnxruntime as ort
import math
import os
import matplotlib.pyplot as plt
from matplotlib import rcParams

# 固定归一化参数
# 改为和 ESP32 一致的 Min-Max 归一化参数
min_vals = np.array([19.95999913, 43.30737291, 19.24016698, 15.17155725], dtype=np.float32)
ranges   = np.array([10.01542005, 34.49400546, 124.10405182, 167.21883223], dtype=np.float32)


def time_encode(hour_index):
    """每小时的周期性时间特征编码"""
    angle = 2 * math.pi * (hour_index % 24) / 24
    return [math.sin(angle), math.cos(angle)]

def cpp_preprocess(data):
    """与 ESP32 C 代码一致的 Min-Max 归一化 + sin/cos 时间特征"""
    data = (data - min_vals) / ranges
    time_features = [time_encode(i) for i in range(len(data))]
    time_features = np.array(time_features, dtype=np.float32)
    data = np.concatenate([data, time_features], axis=1)
    return data.reshape(1, len(data), 6).astype(np.float32)


class ONNXPredictor:
    def __init__(self, model_path):
        if not os.path.exists(model_path):
            raise FileNotFoundError(f"模型文件未找到: {model_path}")
        self.session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])
        self.input_name = self.session.get_inputs()[0].name
        self.output_name = self.session.get_outputs()[0].name
        print(f"[INFO] 模型加载成功: {model_path}")

    def predict(self, input_tensor):
        result = self.session.run([self.output_name], {self.input_name: input_tensor})
        return float(result[0][0][0])  # 输出一个 scalar

# 原始数据（36小时）
raw_data = np.array([
    # [25.496, 72.778, 100.99, 155.567],
    # [24.937, 75.725, 94.624, 128.963],
    # [25.798, 65.791, 82.404, 123.585],
    # [26.749, 71.660, 72.560, 139.818],
    # [25.067, 67.997, 93.499, 136.393],
    # [25.142, 68.459, 89.669, 130.618],
    # [27.030, 68.108, 97.943, 153.472],
    # [26.293, 67.253, 94.788, 158.486],
    # [25.130, 69.943, 101.74, 140.959],
    # [26.216, 67.251, 78.718, 168.527],
    # [25.284, 70.495, 107.23, 115.904],
    # [25.354, 69.468, 91.168, 134.601],

    # [23.861, 50.414, 63.423, 43.360],
    # [24.317, 50.454, 65.589, 73.655],
    # [22.233, 43.307, 48.257, 61.849],
    # [22.085, 51.366, 42.455, 85.831],
    # [23.864, 50.972, 87.338, 90.410],
    # [23.576, 53.896, 50.092, 119.02],
    # [23.468, 57.501, 47.312, 79.410],
    # [23.504, 55.486, 29.280, 74.942],
    # [22.419, 51.456, 51.406, 72.942],
    # [23.273, 49.106, 40.566, 62.639],
    # [23.277, 60.312, 84.476, 77.139],
    # [22.214, 52.941, 74.440, 65.147],

    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],

    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],

    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0],
    [23.0, 45.0, 32.0, 80.0]
], dtype=np.float32)

# 初始化模型
script_dir = os.path.dirname(os.path.abspath(__file__))
model_path = os.path.join(script_dir, "esp32_model.onnx")
predictor = ONNXPredictor(model_path)

# 初始窗口：全零
window_size = 12
input_window = np.zeros((window_size, 4), dtype=np.float32)

# 存储预测概率
probabilities = []

print("\n[预测结果]")

for t in range(len(raw_data)):
    # 滑窗更新：右移一格 + 新数据入列
    input_window = np.roll(input_window, -1, axis=0)
    input_window[-1] = raw_data[t]
    
    input_tensor = cpp_preprocess(input_window)
    probability = predictor.predict(input_tensor)
    probabilities.append(probability)
    print(f"第{t - window_size + 1:02d}~{t:02d}小时 => 清扫概率: {probability * 100:.2f}%")


# 设置中文支持
plt.rcParams['font.sans-serif'] = ['SimHei']  
plt.rcParams['axes.unicode_minus'] = False

# ✅ Python 模拟推理结果（百分比转小数）
python_probs = [
    11.42, 10.37, 12.60, 20.54, 24.39, 31.65, 44.44, 52.60, 78.11, 92.85, 97.15, 98.22,
    98.27, 97.56, 96.76, 95.24, 93.40, 91.08, 91.28, 89.88, 86.96, 84.40, 81.34, 79.60,
    76.53, 77.55, 77.46, 68.14, 57.69, 48.58, 48.26, 49.79, 43.79, 42.06, 37.72, 35.23
]
python_probs = [p / 100 for p in python_probs]

# ✅ ESP32 模拟推理结果（直接使用）
esp32vm_probs = [
    0.1875, 0.1797, 0.2188, 0.3203, 0.4062, 0.5391, 0.7031, 0.7578, 0.8750, 0.9375,
    0.9375, 0.9375, 0.9375, 0.9375, 0.9375, 0.9375, 0.9375, 0.9141, 0.9219, 0.9062,
    0.8828, 0.8594, 0.8359, 0.8359, 0.8125, 0.8047, 0.7734, 0.6953, 0.6094, 0.5469,
    0.5469, 0.5469, 0.5156, 0.5078, 0.4922, 0.5000
]

esp32_probs = [
    0.0156, 0.0156, 0.0234, 0.0938, 0.2109, 0.4688, 0.7188, 0.7891, 0.8516, 0.9141,
    0.9688, 0.9844, 0.9844, 0.9844, 0.9844, 0.9844, 0.9844, 0.9688, 0.9531, 0.8984,
    0.8594, 0.8281, 0.8047, 0.7422, 0.6562, 0.6641, 0.8438, 0.8203, 0.7031, 0.6016,
    0.4375, 0.3672, 0.2969, 0.3359, 0.3438, 0.3125
]

# 时间标签：从 -11 到 +24 小时
time_labels = list(range(-11, 25))

# 绘图
plt.figure(figsize=(14, 6))
plt.plot(time_labels, python_probs, label='Python 推理', marker='o', color='blue')
plt.plot(time_labels, esp32vm_probs, label='ESP32虚拟机模型 推理', marker='x', color='orange')
plt.plot(time_labels, esp32_probs, label='ESP32 推理', marker='x', color='black')

# 当前时间点分界线
plt.axvline(x=0, color='gray', linestyle='--', label='当前时间点')

# 图表美化
plt.title('清扫概率对比图（Python vs ESP32）')
plt.xlabel('时间窗口（小时）')
plt.ylabel('清扫概率')
plt.grid(True)
plt.xticks(time_labels, rotation=45)
plt.legend()
plt.tight_layout()
plt.show()


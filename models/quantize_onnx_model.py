# quantize_sensor_model.py
import os
import numpy as np
import pandas as pd
import joblib  # 新增导入
from sklearn.preprocessing import MinMaxScaler
from ppq import QuantizationSettingFactory
from ppq.api import espdl_quantize_onnx, get_target_platform
from torch.utils.data import Dataset, DataLoader
import torch
import onnx
from onnxsim import simplify

class SensorCalibrationDataset(Dataset):
    def __init__(self, csv_path, scaler_path="scaler_params.pkl", seq_length=12):
        super().__init__()
        self.seq_length = seq_length
        self.features = ['temperature', 'humidity', 'pm25', 'aqi', 'day_sin', 'day_cos']  # 时序特征定义：显式指定传感器特征名称（温度、湿度等）和day_sin/day_cos，确保数据顺序一致性
        
        # 加载数据时保留列名
        df = pd.read_csv(csv_path)
        self.data = df[self.features].copy()  # 通过pd.read_csv加载原始数据，仅保留指定特征列，使用.copy()避免原始数据被修改的风险
        
        # 加载scaler时恢复特征名称
        if not os.path.exists(scaler_path):
            raise FileNotFoundError(f"找不到归一化参数文件：{scaler_path}")
        self.scaler = joblib.load(scaler_path) # 归一化参数恢复：加载预训练的MinMaxScaler参数文件
        self.scaler.feature_names = self.features # 手动设置特征名称以解决某些情况下scaler未保存列名的问题

    def __len__(self): # 返回有效样本数，计算公式为总时间步数 - 序列长度。例如，1000个时间点的数据生成988个长度为12的序列（滑动窗口步长=1）
        return len(self.data) - self.seq_length

    def __getitem__(self, idx):
        seq_df = self.data.iloc[idx:idx+self.seq_length] # 序列截取：使用iloc获取从idx开始的连续seq_length个时间步数据
        assert len(seq_df) == self.seq_length, f"序列长度错误，期望{self.seq_length}，实际{len(seq_df)}"
        scaled_data = self.scaler.transform(seq_df) # 归一化处理：调用scaler.transform对序列标准化，将数据压缩到[0,1]范围
        tensor_data = torch.from_numpy(scaled_data).unsqueeze(0).float() # torch.from_numpy：将NumPy数组转为PyTorch张量
                                                                         # unsqueeze：在第0维增加一个维度，用于表示样本数量，保持形状为[1,12,6]
                                                                         # float：将张量转换为浮点数类型，保持形状为[1,12,6]
        return tensor_data

def quantize_sensor_model():
    # 配置参数
    BATCH_SIZE = 1   # 量化时的批次大小
    SEQ_LENGTH = 12   # 时间序列长度（12小时窗口）
    DEVICE = "cpu"    # 量化计算的执行设备
    TARGET_PLATFORM = "esp32s3" # 目标嵌入式平台
    NUM_OF_BITS = 8   # 量化位数（8-bit量化）
    
    # 文件路径配置
    script_dir = os.path.dirname(os.path.abspath(__file__))  # 获取当前脚本目录
    onnx_path = os.path.join(script_dir, "esp32_model.onnx") # 原始ONNX模型路径
    espdl_path = os.path.join(script_dir, "esp32_model_quantized.espdl")  # 量化后模型输出路径
    calibration_csv = os.path.join(script_dir, "esp32_sensor_data.csv")  # 校准数据集CSV
    scaler_path = os.path.join(script_dir, "scaler_params.pkl")   # 归一化参数文件

    # 模型简化
    model = onnx.load(onnx_path) # 从文件加载ONNX模型
    simplified_model, check = simplify(model) # 执行模型简化（移除冗余节点）
    if check:
        onnx.save(simplified_model, onnx_path)
        print("✅ 模型简化完成")
    else:
        print("⚠️ 模型简化失败，使用原始模型")

    # 准备校准数据集（关键修改：移除num_features参数）
    calibration_dataset = SensorCalibrationDataset(
        csv_path=calibration_csv, # 传感器数据CSV文件
        scaler_path=scaler_path,  # 预训练的归一化参数（scaler）
        seq_length=SEQ_LENGTH     # 时间窗口长度
    )
    
    dataloader = DataLoader(
        dataset=calibration_dataset,
        batch_size=BATCH_SIZE, # 每批加载32个样本
        shuffle=False          # 不打乱数据
    )

    def collate_fn(batch):
        try:
            # 检查批次是否为空
            if len(batch) == 0:
                raise ValueError("接收到空批次，请检查数据集长度是否足够")
                    
            # 检查数据类型和形状
            for i, item in enumerate(batch):
                if not isinstance(item, torch.Tensor):
                    raise TypeError(f"第{i}个元素类型错误，期望torch.Tensor，实际为{type(item)}")
                if item.shape != torch.Size([1, 12, 6]):
                    raise ValueError(f"第{i}个元素形状错误，期望[1,12,6]，实际为{item.shape}")
            
            # 执行拼接
            concatenated = torch.cat(tuple(batch), dim=0)  # 沿第0维拼接 → [32, 12, 6]
            # print(f"拼接后形状: {concatenated.shape}")
            return concatenated
        except Exception as e:
            print(f"数据拼接失败：{str(e)}")
            raise


    # 执行量化
    quant_ppq_graph = espdl_quantize_onnx(
        onnx_import_file=onnx_path,   # 输入ONNX模型路径
        espdl_export_file=espdl_path, # 输出量化模型路径（ESP-DL格式）
        calib_dataloader=dataloader,  # 校准数据集加载器
        calib_steps=32,               # 校准步数（使用32个批次的数据）
        input_shape=[BATCH_SIZE, SEQ_LENGTH, 6], # 模型输入形状
        target=TARGET_PLATFORM,  # 目标硬件平台
        num_of_bits=NUM_OF_BITS,  # 量化位数（8-bit）
        collate_fn=collate_fn,    # 自定义数据拼接函数
        setting=QuantizationSettingFactory.espdl_setting(), # 量化配置（如对称量化、校准方法）
        device=DEVICE, # 执行设备（CPU）
        verbose=1      # 输出详细日志
    )

    print(f"✅ 量化完成，模型已保存至：{espdl_path}")
    return quant_ppq_graph

if __name__ == "__main__":
    quantize_sensor_model()
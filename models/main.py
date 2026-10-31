import numpy as np  # 用于数值计算
import pandas as pd  # 用于处理表格数据
import matplotlib.pyplot as plt  # 用于绘图展示
import tensorflow as tf  # 用于深度学习模型构建与训练
from sklearn.preprocessing import MinMaxScaler  # 用于特征归一化
from sklearn.model_selection import train_test_split  # 用于划分训练集和测试集
from sklearn.metrics import classification_report  # 用于评估分类模型性能
from tensorflow.keras.layers import Input, Dense, Reshape  # 用于构建神经网络结构
from tensorflow.keras.models import Model  # 用于使用Functional API构建模型
from tensorflow.keras.regularizers import l2  # L2正则化防止过拟合
from tensorflow.keras.optimizers import Adam  # 优化器
from tensorflow.keras.callbacks import EarlyStopping  # 训练早停策略
import os  # 用于文件操作

from matplotlib import rcParams  # 设置matplotlib配置
rcParams['font.sans-serif'] = ['Microsoft YaHei', 'WenQuanYi Micro Hei'] # 设置中文字体为黑体
rcParams['axes.unicode_minus'] = False  # 正确显示负号

def visualize_sensor_data(df):
    """可视化传感器数据，包括温湿度、空气质量、清洁需求等"""
    plt.figure(figsize=(14, 10))# 打开一个14英寸宽、10英寸高的电子画板

    # 子图1：温湿度变化
    plt.subplot(3, 1, 1) # 在三行一列的布局中，定位到第1块区域
    plt.plot(df['timestamp'], df['temperature'], label='温度 (℃)', color='tab:red')  # 画红色温度曲线
    plt.plot(df['timestamp'], df['humidity'], label='湿度 (%)', color='tab:blue') # 画蓝色湿度曲线
    plt.title('温度与湿度变化趋势', fontsize=12) # 设置小标题
    plt.ylabel('数值') # Y轴标注
    plt.legend(loc='upper left') # 在左上角显示图例（温度、湿度）
    plt.grid(True, linestyle='--', alpha=0.6)  # 添加虚线网格（透明度60%）

    # 子图2：PM2.5 和 AQI
    plt.subplot(3, 1, 2) # 定位到第2块区域
    plt.plot(df['timestamp'], df['pm25'], label='PM2.5 (ug/m3)', color='tab:green') # 画绿色PM2.5曲线
    plt.plot(df['timestamp'], df['aqi'], label='AQI', color='tab:purple')   # 画紫色AQI曲线
    plt.title('空气质量指标', fontsize=12)
    plt.ylabel('浓度值')
    plt.legend(loc='upper left')
    plt.grid(True, linestyle='--', alpha=0.6)

    # 子图3：清洁需求
    plt.subplot(3, 1, 3)
    plt.fill_between(df['timestamp'], 0, 1,
                     where=df['needs_cleaning'] == 1, # 当needs_cleaning=1时填充
                     color='red', alpha=0.3, # 红色透明背景标记需要清洁的时间段
                     transform=plt.gca().get_xaxis_transform(), # 让红色条覆盖整个X轴范围
                     label='需要清洁')
    plt.plot(df['timestamp'], df['needs_cleaningsmooth'], label='清洁需求概率', color='tab:orange') # 画橙色清洁需求概率曲线
    plt.title('清洁需求判定', fontsize=12)
    plt.xlabel('时间')  # 只在最下方子图显示X轴标签
    plt.yticks([0, 1], ['无需清洁', '需要清洁'])  # 将Y轴0和1映射为文字提示
    plt.grid(True, linestyle='--', alpha=0.6)
    plt.tight_layout() # 自动调整子图间距，防止文字重叠
    plt.show() # 显示画板

def generate_sensor_data(samples=1000, seed=42):
    """生成虚拟传感器数据并包含时间周期编码"""
    np.random.seed(seed) #设置随机数生成器的种子，保证每次重新运行程序生成的随机数相同
    tf.random.set_seed(seed)  # 设置TensorFlow的随机种子
    timestamps = pd.date_range("2023-01-01", periods=samples, freq="h") #从2023年1月1日开始，每小时生成一个时间戳，总数量等于samples参数

    # 生成基础波形数据,大多数 Keras 层（如 Dense, Activation, BatchNormalization）期望输入是浮点型，所以模型得按float型数据进行训练
    base = np.linspace(0, 4 * np.pi, samples) #在0到4π之间均匀生成samples个点（用于生成周期性波动）
    temperature = 25 + 3 * np.sin(base) + np.random.normal(0, 1, samples) # 25 + 3*sin(base) + 随机噪声（标准差1）
    humidity = 60 + 10 * np.cos(base) + np.random.normal(0, 3, samples)   # 60 + 10*cos(base) + 随机噪声（标准差3）
    pm25 = np.clip(80 + 30 * np.sin(base) + np.random.normal(0, 15, samples), 0, 200)# 80 + 30*sin(base) + 随机噪声（标准差15），使用clip限制数值在0-200之间（防止出现负数或不合理高值）
    aqi = np.clip(100 + 40 * np.cos(base) + np.random.normal(0, 20, samples), 0, 200)# 100 + 40*cos(base) + 随机噪声（标准差20），使用clip限制数值在0-200之间（防止出现负数或不合理高值）

    df = pd.DataFrame({
        "timestamp": timestamps,
        "temperature": temperature,  # 温度，21.0 ~ 30.0 之间
        "humidity": humidity,        # 湿度，45.0 ~ 75.0 之间
        "pm25": pm25,                # PM2.5，0 ~ 200 之间
        "aqi": aqi                   # AQI，0 ~ 200 之间
    }) #将生成的数据组合成表格（DataFrame），包含五列数据：时间戳、温度、湿度、PM2.5、AQI

    # 圆代表24h，即2π，每小时就是2π/24，将时间映射到坐标系，用正弦和余弦值表示该点的坐标（X和Y位置），即：0点 → 坐标 (1, 0)（cos(0)=1, sin(0)=0）       6点 → 坐标 (0, 1)（cos(π/2)=0, sin(π/2)=1）
    df['day_sin'] = np.sin(2 * np.pi * df['timestamp'].dt.hour / 24) # （时间正弦值）： sin(2π * 小时 / 24)
    df['day_cos'] = np.cos(2 * np.pi * df['timestamp'].dt.hour / 24) # （时间余弦值）： cos(2π * 小时 / 24)

    # 如果PM2.5或AQI的6小时滑动平均值超过阈值（90或110），用滚动平均避免单次突增误判，转换为0/1标识（1表示需要清洁），清扫决策依据
    df['needs_cleaning'] = ((df['pm25'].rolling(6, min_periods=1).mean() > 90) |
                            (df['aqi'].rolling(6, min_periods=1).mean() > 110)).astype(int)

    # 检查并创建目标路径
    output_dir = os.path.dirname(os.path.abspath(__file__))
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # 写入CSV
    csv_path = os.path.join(output_dir, "esp32_sensor_data.csv")
    df.to_csv(csv_path, index=False)
    print(f"✅ 已生成优化传感器数据（{samples}条），保存至：{csv_path}")

    #生成平滑后的清洁需求曲线（12小时滑动平均），用于可视化时更平滑的曲线显示
    df['needs_cleaningsmooth'] = df['needs_cleaning'].rolling(12).mean()
    visualize_sensor_data(df)
    return df

def build_esp32_model(time_steps, feature_num):
    """构建适用于ESP32的轻量级神经网络模型"""
    inputs = Input(shape=(time_steps, feature_num), name="input")  # 输入层，(batch_size，12，6)，12小时时间窗口，6个特征（温度、湿度、PM2.5、AQI、时间正弦值、时间余弦值）
    x = Reshape((time_steps * feature_num,), name="reshape")(inputs)  # 将输入从3D张量 (batch_size, 12, 6) 转换为2D张量 (batch_size, 72)，合并时间与特征维度。
                                                                      # batch_size：一次训练输入的样本数量，由训练时动态决定（如设置为32或64）
    x = Dense(16, activation='relu', kernel_regularizer=l2(0.01), name="dense1")(x)  # 全连接层1，16个神经元，每个神经元接收72个输入特征，通过线性变换（权重矩阵 W 和偏置 b）生成输出。
                                                                                     # 通过relu激活函，f(x) = max(0, x)数找出关键特征
                                                                                     # L2正则化，kernel_regularizer=l2(0.01)，惩罚权重矩阵的欧式范数，防止模型对训练数据的过拟合（注重细节），增强泛化能力
                                                                                     # 输出形状 (batch_size, 16)
    x = Dense(8, activation='relu', kernel_regularizer=l2(0.01), name="dense2")(x)   # 全连接层2，神经元数量从16降至8，逐步抽象高阶特征，过滤冗余信息
                                                                                     # 输出形状 (batch_size, 8)
    outputs = Dense(1, activation='sigmoid', name="output")(x)  # 输出层， 输出形状 (batch_size, 1)，最终由1个神经元通过sigmoid函数，σ(x) = 1 / (1 + e^{-x})输出0-1之间的概率值
    model = Model(inputs=inputs, outputs=outputs, name="esp32_model")
    return model

def train_optimized_model(df):
    """训练优化模型并返回模型和归一化器
    Args:
        df (pd.DataFrame): 输入传感器数据
    Returns:
        model (tf.keras.Model): 训练好的模型
        scaler (MinMaxScaler): 归一化参数对象
    """
    # 配置参数
    features = ['temperature', 'humidity', 'pm25', 'aqi', 'day_sin', 'day_cos']
    time_steps = 12  # 使用12小时时间窗口
    target = "needs_cleaning"
    
    # 特征归一化，传感器特征（如温度、湿度、PM2.5等）的单位和数值范围差异巨大。例如，温度可能在0-40°C，而AQI指数可能高达500+。归一化将所有特征映射到[0,1]区间，消除量纲对模型的影响，使不同特征对模型贡献均衡.
    scaler = MinMaxScaler() # X_scaled = (X - X_min) / (X_max - X_min)，其中X_min和X_max分别为每个特征的最小值和最大值。例如，若某特征原始范围为[10, 50]，则30会被归一化为(30-10)/(50-10)=0.5
    X = scaler.fit_transform(df[features]) # X 是 输入传感器数据的归一化值，[[0.2, 0.6, 0.35, 0.2, 0, 1], （温度25→0.2，湿度60→0.6，PM2.5 35→0.35 AQI 80→0.2] 6:00(sin(π/2)) 6:00(cos(π/2))）
    y = df[target].values # y 是 二分类标签，表示"是否需要触发清洁"的决策结果

    # 保存归一化参数（新增关键代码）
    import joblib
    scaler_path = os.path.join(os.path.dirname(__file__), "scaler_params.pkl")
    joblib.dump(scaler, scaler_path)
    print(f"✅ 归一化参数已保存至：{scaler_path}")

    # 构造时间序列数据集，将连续12小时（time_steps=12）的数据作为输入，预测下一小时的清洁需求
    #利用24小时周期特征 day_sin/day_cos 增强模型对昼夜模式的学习，窗口大小选择依据：PM2.5和AQI的变化周期通常与半天至一天相关
    X_seq, y_seq = [], []
    for i in range(len(X) - time_steps):
        X_seq.append(X[i:i+time_steps])  # X_seq[0] = X[0:12]，第0~11小时的特征 → 预测第12小时的标签
        y_seq.append(y[i+time_steps])    # y_seq[0] = y[12]，第12小时是否需要清洁
    X_seq, y_seq = np.array(X_seq), np.array(y_seq) # 原始X形状：(1000, 6) （1000个时间点 × 6个特征），转换后X_seq形状：(988, 12, 6) （988个样本 × 12小时窗口 × 6特征）

    # 数据集划分
    X_train, X_test, y_train, y_test = train_test_split(
        X_seq, y_seq, 
        test_size=0.2, # 训练集：前80%时间窗口（如X_train包含0~790小时的数据窗口）， 测试集：后20%时间窗口（如X_test包含791~1000小时的数据窗口）
        random_state=42, # 保证每次划分结果相同
        shuffle=False  # 禁止打乱顺序
    )

    # 构建并编译模型
    model = build_esp32_model(time_steps, len(features))
    model.compile(
        optimizer=Adam(learning_rate=0.001), # Adam优化器：结合动量（一阶矩）和自适应学习率（二阶矩），默认参数（β1=0.9，β2=0.999）适合大多数场景。学习率0.001是深度学习常见初始值，平衡收敛速度与稳定性
        loss="binary_crossentropy", # 二元交叉熵：公式为 Loss = -1/N Σ(y_true·log(y_pred) + (1-y_true)·log(1-y_pred)) 适用于二分类任务，对概率预测的误差敏感（如预测0.9但实际为0时惩罚更大）
        metrics=["accuracy", tf.keras.metrics.Precision(), tf.keras.metrics.Recall()] # Accuracy：全局正确率，适用于类别平衡场景（如正负样本比例接近）
                                                                                      # Precision：正例预测准确率，TP / (TP+FP)，衡量预测为正类的准确性（避免误触发清洁）
                                                                                      # Recall：正例被预测为正例的比例，TP / (TP+FN)，衡量模型对正例的预测能力（避免漏掉正例）
    )

    # 模型训练
    print("\n开始训练...")
    history = model.fit(
        X_train, y_train,
        epochs=15,             # 15次全数据集迭代，配合早停机制可避免过拟合
        batch_size=32,         # 32是常用值，平衡内存效率与梯度稳定性
        validation_split=0.1,  # 10%训练数据作为验证集，用于实时监控泛化能力。
        callbacks=[
            EarlyStopping(     # 持续监控验证集损失（val_loss），若连续5个epoch未改善则终止训练，并回滚到最佳权重
                patience=5, 
                restore_best_weights=True,
                monitor='val_loss',
                mode='min'
            )
        ],
        verbose=1  # 显示进度条，包含每个epoch的损失和指标变化。
    )

    # 模型评估
    print("\n评估结果：")
    loss, acc, precision, recall = model.evaluate(X_test, y_test, verbose=0) # 评估模型在测试集上的损失、准确率、精确率、召回率
                                                                             # Loss（损失值）：模型预测概率与真实标签的差异程度，越小越好，反映模型整体拟合能力
                                                                             # Accuracy（准确率）：模型预测正确的样本占全部样本的比例，反映模型预准确率
                                                                             # Precision（精确率）：预测为"需要清洁"的样本中，实际真实的比例（减少误报），反映模型对正例的预测能力
                                                                             # Recall（召回率）：实际"需要清洁"的样本中，被正确预测的比例（减少漏报），反映模型对正例的预测能力
    print(f"测试准确率: {acc:.2%}")
    print(f"精确率: {precision:.2%}")
    print(f"召回率: {recall:.2%}")
    
    return model, scaler

def export_onnx_model(model):
    """将Keras模型导出为ONNX格式（固定batch_size=1）"""
    try:
        import tf2onnx
        import onnx
    except ImportError:
        print("未找到 tf2onnx 或 onnx 库，正在安装...")
        import subprocess
        subprocess.check_call(["pip", "install", "tf2onnx", "onnx"])
        import tf2onnx
        import onnx

    # 修改输入签名：将None改为1，固定batch_size维度
    input_signature = [tf.TensorSpec(shape=(1, 12, 6), dtype=tf.float32, name="input")]
    
    # 转换模型时添加冻结参数
    model_proto, _ = tf2onnx.convert.from_keras(
        model=model,
        input_signature=input_signature,
        opset=13,
    )
    
    # 保存模型
    onnx_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "esp32_model.onnx")
    onnx.save(model_proto, onnx_path)
    onnx.checker.check_model(onnx.load(onnx_path))
    print(f"✅ ONNX模型已导出至：{onnx_path}（{os.path.getsize(onnx_path)/1024:.1f}KB）")

def realtime_predict(model, scaler, df):
    """演示实时预测流程"""
    features = ['temperature', 'humidity', 'pm25', 'aqi', 'day_sin', 'day_cos']
    latest_data = df[features].tail(12).values  # 获取最近12小时数据
    scaled_data = scaler.transform(latest_data)   # 特征归一化

    prediction = model.predict(np.array([scaled_data]), verbose=0)  # 将数据封装为三维张量（样本数×时间步×特征数），执行预测，关闭日志输出
    clean_prob = prediction[0][0]
    decision = "需要清洁" if clean_prob > 0.5 else "无需清洁" # 阈值设定：0.5为二分类的常用阈值

    print("\n实时预测结果：")
    print(f"清洁概率: {clean_prob:.2%}")
    print(f"决策建议: {decision}")

    # 可视化预测相关数据
    plt.figure(figsize=(10, 4))
    plt.plot(df['timestamp'].tail(6), df['pm25'].tail(6), label='PM2.5') # 绘制最近6个时间点下的PM2.5和AQI
    plt.plot(df['timestamp'].tail(6), df['aqi'].tail(6), label='AQI')
    plt.axvline(x=df['timestamp'].iloc[-1], color='r', linestyle='--', label='当前时刻') # 标记当前时刻
    plt.xticks(rotation=45) # 旋转X轴标签 45度
    plt.legend() # 显示图例
    plt.tight_layout() # 调整布局，防止部分内容被裁剪
    plt.show()

# 主程序入口
if __name__ == "__main__":
    df = generate_sensor_data(500)  # 生成500条虚拟传感器数据
    print("\n=== 开始模型训练 ===")
    model, scaler = train_optimized_model(df)  # 训练模型
    # ========== 打印真实 FEATURE_MIN 和 FEATURE_RANGE ==========
    print("\n=== 归一化参数检查（用于 C++ 或 test_esp32_model.py） ===")
    print("FEATURE_MIN =", scaler.data_min_)
    print("FEATURE_RANGE =", scaler.data_range_)

    export_onnx_model(model)  # 导出ONNX模型
    print("\n=== 实时预测演示 ===")
    realtime_predict(model, scaler, df)  # 执行实时预测演示

# 导入必要库
import argparse  # 命令行参数解析
import shutil    # 文件操作
import struct    # 二进制数据打包/解包
from pathlib import Path  # 路径处理

def struct_pack_string(string, max_len=None):
    """将字符串打包为二进制数据，支持填充空字节"""
    # 计算最大长度
    if max_len == None:
        max_len = len(string)
    else:
        assert len(string) <= max_len  # 验证字符串长度

    # 计算需要填充的空字节数
    left_num = max_len - len(string)
    out_bytes = None
    
    # 逐个字符打包
    for char in string:
        if out_bytes == None:
            out_bytes = struct.pack("b", ord(char))  # 打包单个字符
        else:
            out_bytes += struct.pack("b", ord(char))
    
    # 填充空字节
    for i in range(left_num):
        out_bytes += struct.pack("x")  # 填充空字节
    
    return out_bytes

def get_model_format(filename):
    """读取模型文件前4字节验证格式"""
    with open(filename, "rb") as f:
        data = f.read(4)  # 读取文件头
        format = data.decode("utf-8")  # 解码格式标识
        
        # 格式验证
        if format != "EDL1" and format != "EDL2":
            raise RuntimeError("Wrong model format.")
        return format

def read_data(filename, format):
    """读取二进制数据并根据格式进行对齐填充"""
    data = None
    with open(filename, "rb") as f:
        data = f.read()  # 读取全部数据
    
    # EDL2格式需要16字节对齐
    if format == "EDL2" and len(data) % 16 != 0:
        padding = 16 - len(data) % 16
        data += struct.pack("x") * padding  # 填充空字节对齐
    
    return data

def pack_models(model_path_or_dir, out_file="models.espdl"):
    """核心打包函数"""
    # 处理输入路径
    if len(model_path_or_dir) == 1:
        model_path_or_dir = Path(model_path_or_dir[0])
        if model_path_or_dir.is_file():  # 单个文件直接复制
            shutil.copyfile(model_path_or_dir, out_file)
            return
        else:  # 目录处理
            model_files = sorted(list(model_path_or_dir.glob("*.espdl")))
    else:  # 多文件处理
        model_files = []
        for model_path in sorted(model_path_or_dir):
            model_path = Path(model_path)
            assert model_path.is_file(), "invalid model_path."
            model_files.append(model_path)

    # 验证所有模型格式一致
    model_formats = [get_model_format(file) for file in model_files]
    format = model_formats[0]
    for i in range(1, len(model_formats)):
        if format != model_formats[i]:
            raise RuntimeError("All packed model format should be same.")

    # 准备模型数据
    model_names = []   # 模型名称列表
    model_bins = []    # 模型二进制数据列表
    name_length = 0    # 名称总长度
    
    for model_file in model_files:
        model_names.append(model_file.name)  # 记录文件名
        model_bins.append(read_data(model_file, format))  # 读取数据
        name_length += len(model_file.name)
        print(model_file.name)

    model_num = len(model_names)
    
    # 构建文件头
    if format == "EDL1":
        header_bin = struct_pack_string("PDL1", 4)  # 文件头标识
    else:
        header_bin = struct_pack_string("PDL2", 4)
    
    header_bin += struct.pack("I", model_num)  # 模型数量(uint32)
    
    # 计算偏移量
    name_offset = 4 + 4 + model_num * 12  # 头信息长度
    if format == "EDL1":
        data_offset = name_offset + name_length  # 数据起始偏移
        padding_bin = b""
    else:
        data_offset = (name_offset + name_length + 15) & ~15  # 16字节对齐
        padding_bin = struct.pack("x") * (data_offset - name_offset - name_length)
    
    # 构建名称段和数据段
    name_bin = None
    data_bin = None
    
    for idx, name in enumerate(model_names):
        # 构建名称二进制
        if not name_bin:
            name_bin = struct_pack_string(name, len(name))
        else:
            name_bin += struct_pack_string(name, len(name))
            name_offset += len(model_names[idx - 1])
        
        # 构建数据二进制
        if not data_bin:
            data_bin = model_bins[idx]
        else:
            data_bin += model_bins[idx]
            data_offset += len(model_bins[idx - 1])
        
        # 添加模型元数据到文件头
        header_bin += struct.pack("I", data_offset)  # 数据偏移
        header_bin += struct.pack("I", name_offset)  # 名称偏移
        header_bin += struct.pack("I", len(name))    # 名称长度
    
    # 合并所有数据段
    out_bin = header_bin + name_bin + padding_bin + data_bin
    
    # 写入输出文件
    with open(out_file, "wb") as f:
        f.write(out_bin)

if __name__ == "__main__":
    # 命令行参数解析
    parser = argparse.ArgumentParser(description="esp-dl model package tool")
    parser.add_argument(
        "-m", "--model_path", 
        type=str, 
        nargs="+", 
        help="模型文件路径或目录"
    )
    parser.add_argument(
        "-o",
        "--out_file",
        type=str,
        default="models.espdl",
        help="输出文件名"
    )
    args = parser.parse_args()
    
    # 执行打包
    pack_models(args.model_path, out_file=args.out_file)
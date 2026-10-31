set(src_dirs
            .
            TASK)

set(include_dirs
            .
            TASK)

idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

# 模型路径配置检查
if(CONFIG_MODEL_FILE_PATH)
    message("CONFIG_MODEL_FILE_PATH is defined")  # 调试信息：确认配置存在
endif()
message("CONFIG_MODEL_FILE_PATH: ${CONFIG_MODEL_FILE_PATH}")  # 打印实际模型路径
message("PROJECT_DIR: ${PROJECT_DIR}")  # 显示项目根目录路径

# 模型打包工具配置
set(MVMODEL_EXE ${PROJECT_DIR}/models/pack_model.py)  # Python打包脚本路径
idf_build_get_property(build_dir BUILD_DIR)    # 获取构建目录路径
set(image_file ${build_dir}/espdl_models/models.espdl)  # 生成的模型文件路径
set(MODEL_FILE_PATH ${PROJECT_DIR}/${CONFIG_MODEL_FILE_PATH})  # 完整模型输入路径

# 自定义构建命令（模型预处理）
add_custom_command(
    OUTPUT ${image_file}  # 输出目标文件
    COMMENT "Move and Pack models..."  # 构建时显示的信息
    COMMAND python ${MVMODEL_EXE} 
        --model_path=${MODEL_FILE_PATH}  # 输入模型路径
        --out_file=${image_file}         # 输出文件路径
    DEPENDS ${MODEL_FILE_PATH}  # 声明依赖关系（模型文件变化时触发重建）
    VERBATIM
)

# 创建自定义构建目标
add_custom_target(models_espdl ALL DEPENDS ${image_file})  # 始终构建该目标
add_dependencies(flash models_espdl)  # 将模型打包绑定到flash目标

# 分区表信息获取
partition_table_get_partition_info(size "--partition-name model" "size")   # 获取model分区大小
partition_table_get_partition_info(offset "--partition-name model" "offset")  # 获取分区偏移量

# 分区烧录配置
if("${size}" AND "${offset}")  # 如果分区存在
    esptool_py_flash_to_partition(flash "model" "${image_file}")  # 烧录模型到指定分区
else()  # 分区不存在时的处理
    set(message "Failed to find model in partition table file"
                "Please add a line(Name=model, Size>recommended size in log) to the partition file.")
endif()
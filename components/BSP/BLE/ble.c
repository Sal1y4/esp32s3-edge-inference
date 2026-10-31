#include "ble.h"

#define GATTS_TABLE_TAG "ESP32S3扫地机器人"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SAMPLE_DEVICE_NAME          "ESP32S3扫地机器人"
#define SVC_INST_ID                 0

/* The max length of characteristic value. When the GATT client performs a write or prepare write operation,
*  the data length must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
*/
#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE        1024
#define CHAR_DECLARATION_SIZE       (sizeof(uint8_t))

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_A,
    IDX_CHAR_VAL_A,
    IDX_CHAR_CFG_A,

    IDX_CHAR_B,
    IDX_CHAR_VAL_B,

    IDX_CHAR_C,
    IDX_CHAR_VAL_C,

    HRS_IDX_NB,
};

static uint8_t adv_config_done       = 0;

uint16_t s_heart_rate_handle_table[HRS_IDX_NB];

typedef struct {
    uint8_t                 *prepare_buf;
    int                     prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;

static uint16_t s_conn_id;
static esp_gatt_if_t s_gatts_if;
static uint8_t s_isConnected;

#define BLE_QUEUE_LENGTH  10
#define BLE_QUEUE_ITEM_SIZE sizeof(ble_data_t)

ble_data_t s_data_packet;

static uint8_t raw_adv_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power */
    0x02, 0x0a, 0xeb,
    /* device name */
    0x17, 0x09,  // 长度=23 (0x17), 类型=0x09
    'E','S','P','3','2','S','3',  // ESP32S3 (7字节)
    0xE6,0x89,0xAB,  // 扫
    0xE5,0x9C,0xB0,  // 地
    0xE6,0x9C,0xBA,  // 机
    0xE5,0x99,0xA8,  // 器
    0xE4,0xBA,0xBA   // 人
};

static uint8_t raw_scan_rsp_data[] = {
    /* flags */
    0x02, 0x01, 0x06,
    /* tx power */
    0x02, 0x0a, 0xeb,
    /* service uuid */
    0x03, 0x03, 0xFF, 0x00
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = 0x20,
    .adv_int_max         = 0x40,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst heart_rate_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

/* Service */
static const uint16_t GATTS_SERVICE_UUID_TEST      = 0x00FF;
static const uint16_t GATTS_CHAR_UUID_TEST_A       = 0xFF01;
static const uint16_t GATTS_CHAR_UUID_TEST_B       = 0xFF02;
static const uint16_t GATTS_CHAR_UUID_TEST_C       = 0xFF03;

static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint8_t char_prop_read                =  ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_write               = ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_write_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t heart_measurement_ccc[2]      = {0x00, 0x00};
static const uint8_t char_value[4]                 = {0x11, 0x22, 0x33, 0x44};


/* 完整的GATT属性数据库定义 - 用于构建BLE服务的属性层级 */
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] =
{
     /******************** 服务声明 ********************/
     [IDX_SVC] = {
        {ESP_GATT_AUTO_RSP},  // 自动响应配置
        { 
            ESP_UUID_LEN_16,                     // UUID长度（16位）
            (uint8_t *)&primary_service_uuid,    // 主服务UUID类型（0x2800）
            ESP_GATT_PERM_READ,                  // 只读权限
            sizeof(uint16_t),                    // 属性值最大长度
            sizeof(GATTS_SERVICE_UUID_TEST),     // 当前属性值长度
            (uint8_t *)&GATTS_SERVICE_UUID_TEST  // 服务UUID值
        }
    },

     /******************** 特征A声明 *******************/
     [IDX_CHAR_A] = {
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&character_declaration_uuid, // 特征声明UUID（0x2803）
            ESP_GATT_PERM_READ,                    // 只读权限（声明不可写）
            CHAR_DECLARATION_SIZE,                 // 特征声明结构大小
            CHAR_DECLARATION_SIZE,                 // 当前值长度
            (uint8_t *)&char_prop_read_write_notify // 特征属性字段（读/写/通知）
        }
    },

     /******************** 特征A的值定义 ***************/
     [IDX_CHAR_VAL_A] = {
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&GATTS_CHAR_UUID_TEST_A,    // 自定义特征A的UUID
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, // 读写权限
            GATTS_DEMO_CHAR_VAL_LEN_MAX,          // 最大数据长度
            sizeof(char_value),                   // 当前值长度
            (uint8_t *)char_value                 // 初始特征值
        }
    },

    /**************** 特征A的客户端配置描述符(CCCD) ********/
    [IDX_CHAR_CFG_A] = {
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&character_client_config_uuid, // CCCD UUID（0x2902）
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, // 读写权限
            sizeof(uint16_t),                        // 描述符值长度（2字节）
            sizeof(heart_measurement_ccc),           // 当前值长度
            (uint8_t *)heart_measurement_ccc         // 默认值（通知/指示配置）
        }
    },

    /******************** 特征B声明（只读特征） ************/
    [IDX_CHAR_B] = {
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&character_declaration_uuid,
            ESP_GATT_PERM_READ,
            CHAR_DECLARATION_SIZE,
            CHAR_DECLARATION_SIZE,
            (uint8_t *)&char_prop_read  // 只读属性
        }
    },

     /******************** 特征B的值定义 ********************/
     [IDX_CHAR_VAL_B] = {
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&GATTS_CHAR_UUID_TEST_B, // 自定义特征B的UUID
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, // 实际权限设置矛盾（声明为只读）
            GATTS_DEMO_CHAR_VAL_LEN_MAX,
            sizeof(char_value),
            (uint8_t *)char_value
        }
    },

     /******************** 特征C声明（只写特征） ************/
     [IDX_CHAR_C] = {
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&character_declaration_uuid,
            ESP_GATT_PERM_READ,
            CHAR_DECLARATION_SIZE,
            CHAR_DECLARATION_SIZE,
            (uint8_t *)&char_prop_write  // 只写属性
        }
    },

    /******************** 特征C的值定义 ********************/
    [IDX_CHAR_VAL_C] = {
        {ESP_GATT_AUTO_RSP},
        {
            ESP_UUID_LEN_16,
            (uint8_t *)&GATTS_CHAR_UUID_TEST_C, // 自定义特征C的UUID
            ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, // 权限设置矛盾（声明为只写）
            GATTS_DEMO_CHAR_VAL_LEN_MAX,
            sizeof(char_value),
            (uint8_t *)char_value
        }
    }

};

esp_err_t ble_msg_send(uint8_t * value, uint16_t size)
{
    esp_err_t ret;
    if(s_isConnected == 0)
    {
        ret = ESP_FAIL;
    }
    else
    {
        ret = esp_ble_gatts_send_indicate(
            s_gatts_if, 
            s_conn_id,
            s_heart_rate_handle_table[IDX_CHAR_VAL_A],
            size, 
            value,
            false // 使用notify而非indicate
        );
    }

    return ret;
}

/* BLE GAP事件回调处理函数 */
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        // 原始广播数据设置完成事件
        case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG); // 清除广播配置完成标志位
            if (adv_config_done == 0) {             // 检查所有配置是否完成
                esp_ble_gap_start_advertising(&adv_params); // 启动广播
            }
            break;

        // 扫描响应数据设置完成事件
        case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG); // 清除扫描响应配置标志位
            if (adv_config_done == 0) {
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;

        // 广播启动完成事件
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* 广播启动结果处理 */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "广播启动失败"); // 错误日志
            } else {
                ESP_LOGI(GATTS_TABLE_TAG, "广播启动成功"); // 信息日志
            }
            break;

        // 广播停止完成事件
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(GATTS_TABLE_TAG, "广播停止失败");
            } else {
                ESP_LOGI(GATTS_TABLE_TAG, "广播停止成功");
            }
            break;

        // 连接参数更新事件
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            /* 打印更新后的连接参数详情 */
            ESP_LOGI(GATTS_TABLE_TAG, 
                   "连接参数更新状态: %d\n"
                   "最小间隔: %d (1.25ms)\n"
                   "最大间隔: %d (1.25ms)\n"
                   "当前间隔: %d (1.25ms)\n"
                   "从机延迟: %d 个事件\n"
                   "超时时间: %d (10ms)",
                   param->update_conn_params.status,     // 操作状态
                   param->update_conn_params.min_int,    // 最小连接间隔
                   param->update_conn_params.max_int,    // 最大连接间隔
                   param->update_conn_params.conn_int,   // 当前连接间隔
                   param->update_conn_params.latency,    // 允许的延迟事件数
                   param->update_conn_params.timeout);   // 连接超时时间
            break;

        default:
            break;
    }
}

/* 准备写入事件处理函数 */
void example_prepare_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    // 打印准备写入日志（中文化）
    ESP_LOGI(GATTS_TABLE_TAG, "准备写入数据，句柄=%d，数据长度=%d", 
    param->write.handle, 
    param->write.len);

    esp_gatt_status_t status = ESP_GATT_OK;
    
    if (param->write.offset > PREPARE_BUF_MAX_SIZE) /* 偏移量校验 */
    {
        status = ESP_GATT_INVALID_OFFSET;  // 偏移量超出缓冲区最大限制
        ESP_LOGE(GATTS_TABLE_TAG, "偏移量超标：%d > %d", param->write.offset, PREPARE_BUF_MAX_SIZE);
    } 
    else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE) /* 数据长度校验 */
    {
        status = ESP_GATT_INVALID_ATTR_LEN; // 总长度超过缓冲区容量
        ESP_LOGE(GATTS_TABLE_TAG, "数据超长：%d+%d > %d", param->write.offset, param->write.len, PREPARE_BUF_MAX_SIZE);
    }

    /* 初始化准备缓冲区 */
    if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL) 
    {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE);
        prepare_write_env->prepare_len = 0;

        if (prepare_write_env->prepare_buf == NULL) {
            ESP_LOGE(GATTS_TABLE_TAG, "准备缓冲区内存分配失败");
            status = ESP_GATT_NO_RESOURCES;
        } 
        else 
        {
            ESP_LOGD(GATTS_TABLE_TAG, "已分配%d字节准备缓冲区", PREPARE_BUF_MAX_SIZE);
        }
    }

    /* 需要响应时构造返回数据 */
    if (param->write.need_rsp) 
    {
        esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));

        if (gatt_rsp != NULL) 
        {
            // 填充响应数据结构
            gatt_rsp->attr_value.len = param->write.len;
            gatt_rsp->attr_value.handle = param->write.handle;
            gatt_rsp->attr_value.offset = param->write.offset;
            gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
            memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);

            // 发送响应
            esp_err_t ret = esp_ble_gatts_send_response( gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);

            if (ret != ESP_OK) 
            {
                ESP_LOGE(GATTS_TABLE_TAG, "响应发送失败，错误码：0x%x", ret);
            }

            free(gatt_rsp); // 释放临时响应内存
        } 
        else 
        {
            ESP_LOGE(GATTS_TABLE_TAG, "响应内存分配失败");
            status = ESP_GATT_NO_RESOURCES;
        }
    }

    /* 错误状态提前返回 */
    if (status != ESP_GATT_OK) 
    {
        ESP_LOGW(GATTS_TABLE_TAG, "准备写入异常终止，状态码：0x%x", status);
        return;
    }

    /* 数据写入缓冲区 */
    memcpy(prepare_write_env->prepare_buf + param->write.offset, param->write.value, param->write.len);

    prepare_write_env->prepare_len += param->write.len; // 更新有效数据长度
    ESP_LOGD(GATTS_TABLE_TAG, "已缓存数据长度：%d字节", prepare_write_env->prepare_len);
}

/* 执行写入事件处理函数 */
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    /* 处理执行写入标志 */
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC && prepare_write_env->prepare_buf) 
    {
        // 打印完整准备数据的十六进制内容（调试用）
        ESP_LOGI(GATTS_TABLE_TAG, "执行预写入数据，完整内容（长度=%d）：", prepare_write_env->prepare_len);
        ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }
    else 
    {
        // 取消写入操作日志
        ESP_LOGI(GATTS_TABLE_TAG, "预写入操作已取消");
    }

    /* 清理准备缓冲区 */
    if (prepare_write_env->prepare_buf) 
    {
        free(prepare_write_env->prepare_buf);   // 释放缓冲区内存
        prepare_write_env->prepare_buf = NULL;  // 防止野指针
        ESP_LOGD(GATTS_TABLE_TAG, "已释放准备缓冲区"); // 调试日志
    }
    prepare_write_env->prepare_len = 0;        // 重置数据长度计数器
    ESP_LOGI(GATTS_TABLE_TAG, "准备环境已重置"); // 状态日志
}

/* GATT服务配置文件事件处理函数 */
static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
                                        esp_gatt_if_t gatts_if,
                                        esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    // 服务注册完成事件
    case ESP_GATTS_REG_EVT:
    {
        /* 设置设备名称 */
        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(SAMPLE_DEVICE_NAME);
        if (set_dev_name_ret)
        {
            ESP_LOGE(GATTS_TABLE_TAG, "设备名称设置失败，错误码: 0x%x", set_dev_name_ret);
        }

        /* 配置原始广播数据 */
        esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
        if (raw_adv_ret)
        {
            ESP_LOGE(GATTS_TABLE_TAG, "广播数据配置失败，错误码: 0x%x", raw_adv_ret);
        }
        adv_config_done |= ADV_CONFIG_FLAG; // 设置广播配置完成标志

        /* 配置扫描响应数据 */
        esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_scan_rsp_data, sizeof(raw_scan_rsp_data));
        if (raw_scan_ret)
        {
            ESP_LOGE(GATTS_TABLE_TAG, "扫描响应配置失败，错误码: 0x%x", raw_scan_ret);
        }
        adv_config_done |= SCAN_RSP_CONFIG_FLAG; // 设置扫描响应配置完成标志

        /* 创建属性表 */
        esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_INST_ID);
        s_gatts_if = gatts_if;
        if (create_attr_ret)
        {
            ESP_LOGE(GATTS_TABLE_TAG, "属性表创建失败，错误码: 0x%x", create_attr_ret);
        }
        break;
    }

    // 读取特征值事件
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(GATTS_TABLE_TAG, "[读操作] 特征句柄: %d", param->read.handle);
        break;

    // 写入特征值事件
    case ESP_GATTS_WRITE_EVT:
        if (!param->write.is_prep)
        { // 非准备写入（直接写入）
            // ESP_LOGI(GATTS_TABLE_TAG, "[写操作] 特征句柄: %d，数据长度: %d，数据内容:",
            //          param->write.handle,
            //          param->write.len);
            //ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->write.value, param->write.len); //FIXME:蓝牙数据接收位置
            s_data_packet.handle = param->write.handle;
            s_data_packet.len = BLE_MIN(param->write.len, ESP_GATT_MAX_ATTR_LEN);
            
            memcpy(s_data_packet.value, param->write.value, s_data_packet.len);
            
            if (xQueueSendFromISR(g_ble_recv_msg_queue, &s_data_packet, NULL) != pdTRUE) {
                ESP_LOGE("BLE", "Queue overflow, data loss!");
            }

            /* 处理客户端特性配置描述符（CCCD）写入 */
            if (s_heart_rate_handle_table[IDX_CHAR_CFG_A] == param->write.handle &&
                param->write.len == 2)
            {
                uint16_t descr_value = (param->write.value[1] << 8) | param->write.value[0];

                switch (descr_value)
                {
                case 0x0001: // 启用通知
                    ESP_LOGI(GATTS_TABLE_TAG, "通知功能已启用");
                    // uint8_t notify_data[15];
                    // for (int i = 0; i < sizeof(notify_data); ++i)
                    // {
                    //     notify_data[i] = i % 0xFF; // 生成测试数据
                    // }
                    // // 发送通知（无需确认）
                    // esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id,
                    //                             s_heart_rate_handle_table[IDX_CHAR_VAL_A],
                    //                             sizeof(notify_data), notify_data, false);
                    break;

                case 0x0002: // 启用指示
                    ESP_LOGI(GATTS_TABLE_TAG, "指示功能已启用");
                    uint8_t indicate_data[15];
                    for (int i = 0; i < sizeof(indicate_data); ++i)
                    {
                        indicate_data[i] = i % 0xFF;
                    }
                    // 发送指示（需要客户端确认）
                    esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id,
                                                s_heart_rate_handle_table[IDX_CHAR_VAL_A],
                                                sizeof(indicate_data), indicate_data, true);
                    break;

                case 0x0000: // 禁用通知/指示
                    ESP_LOGI(GATTS_TABLE_TAG, "通知/指示功能已禁用");
                    break;

                default: // 异常值处理
                    ESP_LOGE(GATTS_TABLE_TAG, "非法的描述符值");
                    ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    break;
                }
            }

            /* 发送写响应（如果需要） */
            if (param->write.need_rsp)
            {
                esp_ble_gatts_send_response(gatts_if, param->write.conn_id,
                                            param->write.trans_id, ESP_GATT_OK, NULL);
            }
        }
        else
        { // 准备写入处理
            example_prepare_write_event_env(gatts_if, &prepare_write_env, param);
        }
        break;

    // 执行写入事件（长数据提交）
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TABLE_TAG, "[执行写入] 总数据长度: %d", prepare_write_env.prepare_len);
        example_exec_write_event_env(&prepare_write_env, param);
        break;

    // MTU更新事件
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TABLE_TAG, "MTU更新至: %d 字节", param->mtu.mtu);
        break;

    // 配置确认事件 //FIXME:向主机发送数据时会打印
    case ESP_GATTS_CONF_EVT:
        // ESP_LOGI(GATTS_TABLE_TAG, "配置确认状态: %s，特征句柄: %d",
        //          (param->conf.status == ESP_GATT_OK) ? "成功" : "失败",
        //          param->conf.handle);
        break;

    // 服务启动事件
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TABLE_TAG, "服务启动%s，服务句柄: %d",
                 (param->start.status == ESP_GATT_OK) ? "成功" : "失败",
                 param->start.service_handle);
        break;

    // 设备连接事件
    case ESP_GATTS_CONNECT_EVT:
    {
        ESP_LOGI(GATTS_TABLE_TAG, "设备已连接，连接ID: %d，MAC地址:", param->connect.conn_id);
        ESP_LOG_BUFFER_HEX(GATTS_TABLE_TAG, param->connect.remote_bda, 6);

        /* 更新连接参数（iOS需特别注意参数范围） */
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_params.latency = 0;    // 从机延迟事件数
        conn_params.max_int = 0x20; // 最大间隔 0x20*1.25ms = 40ms
        conn_params.min_int = 0x10; // 最小间隔 0x10*1.25ms = 20ms
        conn_params.timeout = 400;  // 超时时间 400*10ms = 4000ms
        esp_ble_gap_update_conn_params(&conn_params);
        s_conn_id = param->connect.conn_id;
        s_isConnected = 1;
        break;
    }

    // 设备断开事件
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(GATTS_TABLE_TAG, "设备已断开，原因: 0x%02x", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params); // 重启广播
        s_conn_id = 0;
        s_isConnected = 0;
        break;

    // 属性表创建完成事件
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    {
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGE(GATTS_TABLE_TAG, "属性表创建失败，状态码: 0x%x", param->add_attr_tab.status);
        }
        else if (param->add_attr_tab.num_handle != HRS_IDX_NB)
        {
            ESP_LOGE(GATTS_TABLE_TAG, "属性表异常，实际句柄数(%d)≠预期数(%d)",
                     param->add_attr_tab.num_handle, HRS_IDX_NB);
        }
        else
        {
            ESP_LOGI(GATTS_TABLE_TAG, "属性表创建成功，总句柄数: %d", param->add_attr_tab.num_handle);
            memcpy(s_heart_rate_handle_table, param->add_attr_tab.handles,
                   sizeof(s_heart_rate_handle_table));                       // 保存特征句柄
            esp_ble_gatts_start_service(s_heart_rate_handle_table[IDX_SVC]); // 启动服务
        }
        break;
    }

    // 以下事件暂不处理但保留结构
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_DELETE_EVT:
    default:
        ESP_LOGD(GATTS_TABLE_TAG, "未处理事件: %d", event);
        break;
    }
}

/* GATT服务事件统一回调处理函数 */
static void gatts_event_handler(esp_gatts_cb_event_t event,
                                esp_gatt_if_t gatts_if,
                                esp_ble_gatts_cb_param_t *param)
{
    /* 处理应用注册事件 */
    if (event == ESP_GATTS_REG_EVT)
    { // 应用注册完成事件
        if (param->reg.status == ESP_GATT_OK)
        { // 注册成功
            // 将协议栈分配的gatts_if存储到对应的profile结构体
            heart_rate_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        }
        else
        { // 注册失败处理
            ESP_LOGE(GATTS_TABLE_TAG, "应用注册失败，APP_ID:0x%04x，状态码:%d",
                     param->reg.app_id,
                     param->reg.status);
            return; // 直接返回，不进行后续处理
        }
    }

    /* 多Profile事件分发机制 */
    do
    { // 使用do-while包裹实现代码块作用域
        for (int idx = 0; idx < PROFILE_NUM; idx++)
        { // 遍历所有Profile实例
            /* 判断是否需要处理该Profile */
            if (gatts_if == ESP_GATT_IF_NONE || // 无指定接口（广播事件）
                gatts_if == heart_rate_profile_tab[idx].gatts_if)
            { // 匹配当前Profile接口

                /* 调用已注册的Profile回调 */
                if (heart_rate_profile_tab[idx].gatts_cb)
                { // 确保回调函数存在
                    heart_rate_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0); // 仅执行一次的循环结构，用于代码块封装
}

/* 应用程序主入口函数 */
void ble_init(void)
{
    esp_err_t ret; // 错误状态存储变量

    g_ble_recv_msg_queue = xQueueCreate(BLE_QUEUE_LENGTH, BLE_QUEUE_ITEM_SIZE);
    configASSERT(g_ble_recv_msg_queue); // 生产环境应替换为错误处理

    /* 释放经典蓝牙内存（仅使用BLE模式）*/
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    /* 蓝牙控制器初始化配置 */
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[错误] 蓝牙控制器初始化失败，错误码：%s", 
               esp_err_to_name(ret));
        return;
    }

    /* 启用BLE控制器 */
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[错误] 蓝牙控制器启用失败，错误码：%s",
               esp_err_to_name(ret));
        return;
    }

    /* 初始化蓝牙协议栈 */
    ret = esp_bluedroid_init();
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[错误] 蓝牙协议栈初始化失败，错误码：%s",
               esp_err_to_name(ret));
        return;
    }

    /* 启用蓝牙协议栈 */
    ret = esp_bluedroid_enable();
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[错误] 蓝牙协议栈启用失败，错误码：%s",
               esp_err_to_name(ret));
        return;
    }

    /* 注册GATT事件回调 */
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[错误] GATT回调注册失败，错误码：0x%x", ret);
        return;
    }

    /* 注册GAP事件回调 */
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[错误] GAP回调注册失败，错误码：0x%x", ret);
        return;
    }

    /* 注册GATT应用程序 */
    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[错误] GATT应用注册失败，错误码：0x%x", ret);
        return;
    }

    /* 设置本地MTU（最大传输单元）*/
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret != ESP_OK) {
        ESP_LOGE(GATTS_TABLE_TAG, "[警告] 本地MTU设置失败，错误码：0x%x", 
               local_mtu_ret);
        // 注意：MTU设置失败不会影响后续操作，但可能限制数据传输量
    }

    ESP_LOGI(GATTS_TABLE_TAG, "系统初始化完成，BLE服务已就绪");
}
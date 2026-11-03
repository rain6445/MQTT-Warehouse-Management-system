#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

// 蓝牙相关的标准引用
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "bt_server.h"

#define TAG "BT_SERVER"
#define LED_PIN 2  // 控制板载 LED

// 定义
#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0
#define DEVICE_NAME "ESP32_BTXSY_1139SERVER"
#define MAX_CONN_NUM 3
#define ADV_CONFIG_FLAG (1 << 0)
#define SCAN_RSP_CONFIG_FLAG (1 << 1)

// 自定义 UUID
static uint8_t service_uuid128[16] = {
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

static uint8_t char_uuid128[16] = {
    0xff, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80,
    0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};

// 全局变量
static uint16_t g_service_handle = 0;
static esp_gatt_if_t g_gatts_if = 0;
static uint16_t g_conn_id = 0;
static bool g_is_connected = false;

// 回调函数
static void (*g_receive_callback)(uint8_t *, uint16_t) = NULL;

// 特征值句柄
static uint16_t g_char_handle = 0;

// GAP 事件处理
static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    // 修复：删除了未使用的变量 err
    
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            ESP_LOGI(TAG, "ADV data set complete");
            // 开始广播
            esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
                .adv_int_min        = 0x20,
                .adv_int_max        = 0x40,
                .adv_type           = ADV_TYPE_IND,
                .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
                .channel_map        = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
            });
            break;
            
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising start failed");
            } else {
                ESP_LOGI(TAG, "Advertising started successfully");
            }
            break;
            
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "Update connection params status = %d", param->update_conn_params.status);
            break;
            
        default:
            break;
    }
}

// GATTS 事件处理
static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:
            ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", 
                    param->reg.status, param->reg.app_id);
            if (param->reg.status == ESP_GATT_OK) {
                g_gatts_if = gatts_if;
                
                // 设置设备名
                esp_ble_gap_set_device_name("1138_ESP32");
                
                // 配置广播数据
                esp_ble_gap_config_adv_data(&(esp_ble_adv_data_t){
                    .set_scan_rsp = false,
                    .include_name = true,
                    .include_txpower = true,
                    .min_interval = 0x0006,
                    .max_interval = 0x0010,
                    .appearance = 0x00,
                    .manufacturer_len = 0,
                    .p_manufacturer_data = NULL,
                    .service_data_len = 0,
                    .p_service_data = NULL,
                    .service_uuid_len = 16,
                    .p_service_uuid = service_uuid128,
                    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
                });
                
                // 创建服务
                esp_gatt_srvc_id_t service_id;
                service_id.is_primary = true;
                service_id.id.inst_id = 0x00;
                service_id.id.uuid.len = ESP_UUID_LEN_128;
                memcpy(service_id.id.uuid.uuid.uuid128, service_uuid128, ESP_UUID_LEN_128);
                
                esp_ble_gatts_create_service(gatts_if, &service_id, 4);
            } else {
                ESP_LOGE(TAG, "Reg app failed, app_id %04x, status %d",
                        param->reg.app_id, param->reg.status);
            }
            break;
            
        case ESP_GATTS_CREATE_EVT:
            ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d, service_handle %d",
                    param->create.status, param->create.service_handle);
            if (param->create.status == ESP_GATT_OK) {
                g_service_handle = param->create.service_handle;
                
                // 启动服务
                esp_ble_gatts_start_service(g_service_handle);
                
                // 添加特征值
                esp_bt_uuid_t char_uuid;
                char_uuid.len = ESP_UUID_LEN_128;
                memcpy(char_uuid.uuid.uuid128, char_uuid128, ESP_UUID_LEN_128);
                
                esp_ble_gatts_add_char(g_service_handle, &char_uuid,
                                      ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                      ESP_GATT_CHAR_PROP_BIT_READ | 
                                      ESP_GATT_CHAR_PROP_BIT_WRITE |
                                      ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                                      NULL, NULL);
            }
            break;
            
        case ESP_GATTS_ADD_CHAR_EVT:
            ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d, attr_handle %d, service_handle %d",
                    param->add_char.status, param->add_char.attr_handle, 
                    param->add_char.service_handle);
            if (param->add_char.status == ESP_GATT_OK) {
                g_char_handle = param->add_char.attr_handle;
            }
            break;
            
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            g_conn_id = param->connect.conn_id;
            g_is_connected = true;
            
            // 更新连接参数
            esp_ble_conn_update_params_t conn_params = {
                .bda = {0},
                .latency = 0,
                .max_int = 0x20,
                .min_int = 0x10,
                .timeout = 400,
            };
            memcpy(conn_params.bda, param->connect.remote_bda, ESP_BD_ADDR_LEN);
            esp_ble_gap_update_conn_params(&conn_params);
            break;
            
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            g_is_connected = false;
            
            // 重新开始广播
            esp_ble_gap_start_advertising(&(esp_ble_adv_params_t){
                .adv_int_min        = 0x20,
                .adv_int_max        = 0x40,
                .adv_type           = ADV_TYPE_IND,
                .own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
                .channel_map        = ADV_CHNL_ALL,
                .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
            });
            break;
            
        case ESP_GATTS_WRITE_EVT:
            // 修复：使用 %lu 打印 trans_id (uint32_t)，使用 %u 打印 conn_id 和 handle
            ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %u, trans_id %lu, handle %u",
                    param->write.conn_id, (unsigned long)param->write.trans_id, param->write.handle);
            
            // 处理接收到的数据
            if (param->write.is_prep == false) {
                if (param->write.handle == g_char_handle) {
                    if (param->write.len > 0) {
                        ESP_LOGI(TAG, "Received data, len=%d", param->write.len);
                        
                        // 打印接收到的数据
                        for (int i = 0; i < param->write.len; i++) {
                            ESP_LOGI(TAG, "Data[%d]: 0x%02x", i, param->write.value[i]);
                        }
                        
                        // 控制 LED（兼容字符和二进制）
                        uint8_t data = param->write.value[0];
                        if (data == '1' || data == 0x01) {
                            ESP_LOGI(TAG, "LED ON");
                            gpio_set_level(LED_PIN, 1);
                        } else if (data == '0' || data == 0x00) {
                            ESP_LOGI(TAG, "LED OFF");
                            gpio_set_level(LED_PIN, 0);
                        }
                        
                        // 调用用户回调函数
                        if (g_receive_callback != NULL) {
                            g_receive_callback((uint8_t *)param->write.value, param->write.len);
                        }
                    }
                }
                
                // 发送响应
                if (param->write.need_rsp) {
                    esp_ble_gatts_send_response(g_gatts_if, param->write.conn_id,
                                                param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }
            break;
            
        case ESP_GATTS_READ_EVT:
            // 修复：使用 %lu 打印 trans_id
            ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %u, trans_id %lu, handle %u",
                    param->read.conn_id, (unsigned long)param->read.trans_id, param->read.handle);
            break;
            
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
            
        default:
            break;
    }
}

// GATTS 事件分发
static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, 
                                esp_ble_gatts_cb_param_t *param)
{
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gatts_profile_event_handler(event, gatts_if, param);
        }
    } else {
        gatts_profile_event_handler(event, gatts_if, param);
    }
}

// 公共接口实现

void bt_server_init(void)
{
    ESP_LOGI(TAG, "Initializing BLE Server...");
    
    // 1. 初始化 LED GPIO
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_PIN, 0);
    
    // 2. 初始化 NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // 3. 初始化蓝牙控制器
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
    
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(TAG, "Bluetooth controller enable failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // 4. 初始化 Bluedroid
    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid init failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(TAG, "Bluedroid enable failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // 5. 注册回调函数
    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GATTS register callback failed: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret) {
        ESP_LOGE(TAG, "GAP register callback failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // 6. 注册应用
    ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
    if (ret) {
        ESP_LOGE(TAG, "GATTS app register failed: %s", esp_err_to_name(ret));
        return;
    }
    
    // 7. 设置 MTU 大小
    ret = esp_ble_gatt_set_local_mtu(500);
    if (ret) {
        ESP_LOGE(TAG, "Set local MTU failed: %s", esp_err_to_name(ret));
    }
    
    ESP_LOGI(TAG, "BLE Server initialized successfully");
}

bool bt_server_send_data(const uint8_t *data, uint16_t length)
{
    if (!g_is_connected || g_conn_id == 0) {
        // ESP_LOGE(TAG, "No device connected"); // 降低日志级别以避免刷屏
        return false;
    }
    
    if (length == 0 || data == NULL) {
        ESP_LOGE(TAG, "Invalid data");
        return false;
    }
    
    esp_err_t ret = esp_ble_gatts_send_indicate(g_gatts_if, g_conn_id, 
                                                g_char_handle, length, 
                                                (uint8_t *)data, false);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Send data failed: %s", esp_err_to_name(ret));
        return false;
    }
    
    ESP_LOGI(TAG, "Data sent successfully, length: %d", length);
    return true;
}

bool bt_server_is_connected(void)
{
    return g_is_connected;
}

void bt_server_set_receive_callback(void (*callback)(uint8_t *, uint16_t))
{
    g_receive_callback = callback;
}

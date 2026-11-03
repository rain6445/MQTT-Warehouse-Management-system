#include "mqtt_client.h" 
#include "mqtt_component.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static const char *TAG = "MQTT_COMP";

// 静态变量定义
static esp_mqtt_client_handle_t mqtt_client = NULL;
static bool mqtt_connected = false;
static mqtt_data_callback_t data_callback = NULL;
static char student_id[32] = {0};

// 发布任务函数 (每2秒发布一次数据到 /temp)
static void publish_task(void *pvParameters)
{
    char publish_topic[64];
    snprintf(publish_topic, sizeof(publish_topic), "%s/temp", student_id);
    
    while (1) {
        if (mqtt_connected) {
            // 这里为了演示发布了学号，实际应用中可以发布传感器数据
            int msg_id = esp_mqtt_client_publish(mqtt_client, publish_topic, 
                                                 student_id, 0, 0, 0);
            if (msg_id >= 0) {
                ESP_LOGD(TAG, "发布到主题 %s, 消息ID=%d", publish_topic, msg_id);
            }
        }
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

// 错误日志函数
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "错误 %s: 0x%x", message, error_code);
    }
}

// MQTT事件处理函数
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, 
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT连接已建立");
            mqtt_connected = true;
            break; 
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT连接已断开");
            mqtt_connected = false;
            break;
            
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGD(TAG, "订阅成功, 消息ID=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(TAG, "发布成功, 消息ID=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGD(TAG, "收到MQTT数据");
            
            // 如果有回调函数，调用它
            if (data_callback != NULL) {
                // 复制数据以确保安全（加上字符串结束符）
                char *topic = strndup(event->topic, event->topic_len);
                char *data = strndup(event->data, event->data_len);
                
                if (topic && data) {
                    data_callback(topic, data, event->topic_len, event->data_len);
                }
                
                if (topic) free(topic);
                if (data) free(data);
            }
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT发生错误");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                log_error_if_nonzero("esp-tls报告", event->error_handle->esp_tls_last_esp_err);
                log_error_if_nonzero("tls栈报告", event->error_handle->esp_tls_stack_err);
            }
            break;
            
        default:
            break;
    }
}

// 初始化MQTT组件
esp_err_t mqtt_component_init(const mqtt_config_t* config)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "配置不能为空");
        return ESP_ERR_INVALID_ARG;
    }
    
    // 保存学号
    strncpy(student_id, config->student_id, sizeof(student_id) - 1);
    
    ESP_LOGI(TAG, "初始化MQTT组件");
    ESP_LOGI(TAG, "学号: %s", config->student_id);
    ESP_LOGI(TAG, "服务器: %s", config->broker_url);
    ESP_LOGI(TAG, "客户端ID: %s", config->client_id);
    
    // 配置MQTT客户端
    // 修改 mqtt_component.c 中的配置部分
    esp_mqtt_client_config_t mqtt_cfg = {
    // 1. Broker 地址配置
        .broker.address.uri = config->broker_url,

    // 2. 认证信息配置 (Client ID)
        .credentials.client_id = config->client_id,

    // 3. 会话配置 (解决 "僵尸连接" 的关键)
        .session = {
            .keepalive = 60,
        // disable_clean_session = 0 表示 "开启 Clean Session"
        // 意味着每次连接都清除旧消息，不接收离线消息，防止缓冲区溢出
        .disable_clean_session = 0, 
        },

    // 4. 网络配置
        .network = {
            .reconnect_timeout_ms = 5000,
        },

    // 5. 缓冲区配置 (增大缓冲区以防溢出)
        .buffer = {
            .size = 4096,     // 接收缓冲区 (默认是1024，建议改大)
            .out_size = 4096, // 发送缓冲区
        }
    };

    
    // 创建MQTT客户端
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT客户端初始化失败");
        return ESP_FAIL;
    }
    
    // 注册事件处理器
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    
    return ESP_OK;
}

// 启动MQTT连接
esp_err_t mqtt_component_start(void)
{
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT客户端未初始化");
        return ESP_ERR_INVALID_STATE;
    }
    
    esp_err_t err = esp_mqtt_client_start(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT启动失败: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "MQTT连接已启动");
    return ESP_OK;
}

// 停止MQTT连接
esp_err_t mqtt_component_stop(void)
{
    if (mqtt_client == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    
    ESP_LOGI(TAG, "开始停止MQTT连接...");
    
    // 先断开连接（如果已连接）
    if (mqtt_connected) {
        esp_err_t err = esp_mqtt_client_disconnect(mqtt_client);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "MQTT断开连接失败: %s", esp_err_to_name(err));
        } else {
            ESP_LOGI(TAG, "MQTT连接已断开");
        }
        
        // 等待连接断开
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    
    // 停止客户端
    esp_err_t err = esp_mqtt_client_stop(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT客户端停止失败: %s", esp_err_to_name(err));
        return err;
    }
    
    ESP_LOGI(TAG, "MQTT客户端已停止");
    
    // 销毁客户端
    err = esp_mqtt_client_destroy(mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT客户端销毁失败: %s", esp_err_to_name(err));
        return err;
    }
    
    mqtt_client = NULL;
    mqtt_connected = false;
    
    ESP_LOGI(TAG, "MQTT连接已完全停止并清理");
    return ESP_OK;
}

// 发布消息
esp_err_t mqtt_component_publish(const char* topic, const char* data, int qos, int retain)
{
    if (mqtt_client == NULL || !mqtt_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_publish(mqtt_client, topic, data, 0, qos, retain);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

// 订阅主题
esp_err_t mqtt_component_subscribe(const char* topic, int qos)
{
    if (mqtt_client == NULL || !mqtt_connected) {
        return ESP_ERR_INVALID_STATE;
    }
    
    int msg_id = esp_mqtt_client_subscribe(mqtt_client, topic, qos);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

// 设置数据接收回调函数
void mqtt_component_set_callback(mqtt_data_callback_t callback)
{
    data_callback = callback;
}

// 获取MQTT连接状态
bool mqtt_component_is_connected(void)
{
    return mqtt_connected;
}

// 获取MQTT客户端句柄
esp_mqtt_client_handle_t mqtt_component_get_client(void)
{
    return mqtt_client;
}

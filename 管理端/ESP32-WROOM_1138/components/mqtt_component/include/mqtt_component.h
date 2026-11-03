#ifndef MQTT_COMPONENT_H
#define MQTT_COMPONENT_H

#include "esp_err.h"
#include "mqtt_client.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 配置结构体
typedef struct {
    char student_id[32];
    char broker_url[128];
    char client_id[64];
} mqtt_config_t;

// 回调函数类型定义
typedef void (*mqtt_data_callback_t)(const char* topic, const char* data, int topic_len, int data_len);

// 初始化MQTT组件
esp_err_t mqtt_component_init(const mqtt_config_t* config);

// 启动MQTT连接
esp_err_t mqtt_component_start(void);

// 停止MQTT连接
esp_err_t mqtt_component_stop(void);

// 发布消息
esp_err_t mqtt_component_publish(const char* topic, const char* data, int qos, int retain);

// 订阅主题
esp_err_t mqtt_component_subscribe(const char* topic, int qos);

// 设置数据接收回调函数
void mqtt_component_set_callback(mqtt_data_callback_t callback);

// 获取MQTT连接状态
bool mqtt_component_is_connected(void);

// 获取MQTT客户端句柄（供高级用户使用）
esp_mqtt_client_handle_t mqtt_component_get_client(void);

#ifdef __cplusplus
}
#endif

#endif // MQTT_COMPONENT_H
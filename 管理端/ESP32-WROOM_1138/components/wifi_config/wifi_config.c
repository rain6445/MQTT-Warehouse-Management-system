#include "wifi_config.h"
#include "nvs_flash.h"
#include <string.h>

// 默认WiFi凭证
#define DEFAULT_WIFI_SSID "YOUR_WIFI_SSID"
#define DEFAULT_WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// NVS相关定义
#define NVS_NAMESPACE "iot_config"
#define NVS_KEY_SSID "wifi_ssid"
#define NVS_KEY_PWD "wifi_password"
#define NVS_KEY_CITY "default_city"

static const char *TAG = "WIFI_CONFIG";
static user_wifi_config_t current_config;
static bool wifi_connected = false;
static esp_event_handler_instance_t instance_any_id;
static esp_event_handler_instance_t instance_got_ip;

// WiFi事件处理函数
static void wifi_event_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
        ESP_LOGI(TAG, "WiFi STA启动，开始连接...");
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_connected = false;
        ESP_LOGI(TAG, "WiFi断开，尝试重新连接...");
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        wifi_connected = true;
        ESP_LOGI(TAG, "获得IP地址: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

/**
 * @brief 从NVS（非易失性存储）读取WiFi配置信息
 * 
 * 此函数从ESP32的NVS存储中读取WiFi配置，包括SSID、密码和默认城市信息。
 * 如果NVS中不存在相应的配置项，函数会使用默认值进行回退。
 * 
 * @note 读取顺序和回退策略：
 * 1. 首先尝试打开NVS命名空间，如果失败则使用默认配置并返回
 * 2. 依次读取SSID、密码和城市配置，任何一项读取失败都使用默认值
 * 3. 所有配置项都支持回退机制，确保系统始终有可用的配置
 * 
 * @warning 此函数在初始化阶段被调用，如果NVS存储损坏或不存在，
 *          系统将使用硬编码的默认配置继续运行
 * 
 * @see wifi_save_config() - 对应的配置保存函数
 * @see wifi_set_config() - 配置设置函数
 * 
 * @example
 * // 在系统启动时调用
 * wifi_read_config();
 * // 之后可以通过wifi_get_config()获取读取的配置
 * 
 * @return 无返回值，配置结果存储在全局current_config变量中
 */
void wifi_read_config(void)
{
    nvs_handle_t nvs_handle;
    // 以只读模式打开NVS命名空间
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    
    // 如果NVS打开失败，使用默认配置
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "NVS打开失败，使用默认配置");
        // 设置默认SSID
        strncpy(current_config.ssid, DEFAULT_WIFI_SSID, sizeof(current_config.ssid) - 1);
        // 设置默认密码
        strncpy(current_config.password, DEFAULT_WIFI_PASSWORD, sizeof(current_config.password) - 1);
        // 清空城市配置
        current_config.city[0] = '\0';
        return;
    }
    
    // 读取SSID配置
    size_t len = sizeof(current_config.ssid);
    if (nvs_get_str(nvs_handle, NVS_KEY_SSID, current_config.ssid, &len) != ESP_OK) {
        ESP_LOGW(TAG, "未找到SSID配置，使用默认值");
        // SSID读取失败，使用默认值
        strncpy(current_config.ssid, DEFAULT_WIFI_SSID, sizeof(current_config.ssid) - 1);
    }
    
    // 读取密码配置
    len = sizeof(current_config.password);
    if (nvs_get_str(nvs_handle, NVS_KEY_PWD, current_config.password, &len) != ESP_OK) {
        ESP_LOGW(TAG, "未找到密码配置，使用默认值");
        // 密码读取失败，使用默认值
        strncpy(current_config.password, DEFAULT_WIFI_PASSWORD, sizeof(current_config.password) - 1);
    }
    
    // 读取城市配置
    len = sizeof(current_config.city);
    if (nvs_get_str(nvs_handle, NVS_KEY_CITY, current_config.city, &len) != ESP_OK) {
        ESP_LOGW(TAG, "未找到默认城市配置");
        // 城市配置读取失败，清空城市字段
        current_config.city[0] = '\0';
    }
    
    // 关闭NVS句柄
    nvs_close(nvs_handle);
    // 记录读取的配置信息（不记录密码以保护安全）
    ESP_LOGI(TAG, "读取配置: SSID=%s, 城市=%s", current_config.ssid, current_config.city);
}

// 保存配置到NVS
void wifi_save_config(void)
{
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS打开失败: %s", esp_err_to_name(err));
        return;
    }
    
    err = nvs_set_str(nvs_handle, NVS_KEY_SSID, current_config.ssid);
    err |= nvs_set_str(nvs_handle, NVS_KEY_PWD, current_config.password);
    err |= nvs_set_str(nvs_handle, NVS_KEY_CITY, current_config.city);
    
    if (err == ESP_OK) {
        err = nvs_commit(nvs_handle);
        ESP_LOGI(TAG, "配置保存成功");
    } else {
        ESP_LOGE(TAG, "配置保存失败");
    }
    
    nvs_close(nvs_handle);
}

/**
 * @brief 设置WiFi配置并重新连接
 * 
 * 此函数用于动态更新WiFi的SSID和密码配置，并立即应用新的配置重新连接。
 * 如果传入的ssid或password参数为NULL，则保持原有的对应配置不变。
 * 
 * @param ssid 新的WiFi网络名称（SSID），如果为NULL则保持原有SSID
 * @param password 新的WiFi密码，如果为NULL则保持原有密码
 * 
 * @note 函数执行流程：
 * 1. 更新内部配置结构体中的SSID和密码
 * 2. 创建新的WiFi配置结构体并设置认证模式为WPA2-PSK
 * 3. 断开当前WiFi连接
 * 4. 应用新的WiFi配置
 * 5. 重新发起WiFi连接
 * 
 * @warning 此函数会立即断开当前连接并尝试重新连接，可能导致短暂网络中断
 * 
 * @example
 * // 更改SSID和密码
 * wifi_set_config("MyNewWiFi", "newpassword123");
 * 
 * // 仅更改密码，保持SSID不变
 * wifi_set_config(NULL, "newpassword123");
 */
void wifi_set_config(const char *ssid, const char *password)
{
    // 更新SSID配置（如果提供了新SSID）
    if (ssid) {
        // 安全拷贝SSID，确保字符串以null结尾
        strncpy(current_config.ssid, ssid, sizeof(current_config.ssid) - 1);
        current_config.ssid[sizeof(current_config.ssid) - 1] = '\0';
    }
    
    // 更新密码配置（如果提供了新密码）
    if (password) {
        // 安全拷贝密码，确保字符串以null结尾
        strncpy(current_config.password, password, sizeof(current_config.password) - 1);
        current_config.password[sizeof(current_config.password) - 1] = '\0';
    }
    
    // 重新配置WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",           // 初始化SSID为空字符串
            .password = "",       // 初始化密码为空字符串
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,  // 设置认证模式为WPA2-PSK
        },
    };
    
    // 将配置拷贝到WiFi配置结构体
    strncpy((char*)wifi_config.sta.ssid, current_config.ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, current_config.password, sizeof(wifi_config.sta.password));
    
    // 应用新的WiFi配置
    esp_wifi_disconnect();                    // 断开当前WiFi连接
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);  // 设置新的WiFi配置
    esp_wifi_connect();                       // 重新连接WiFi
}

// 获取当前配置
void wifi_get_config(user_wifi_config_t *config)
{
    if (config) {
        memcpy(config, &current_config, sizeof(user_wifi_config_t));
    }
}

// 初始化WiFi
void wifi_config_init(void)
{
    ESP_LOGI(TAG, "初始化WiFi配置...");
    
    // 读取配置
    wifi_read_config();
    
    // 创建默认WiFi STA网络接口
    
    esp_netif_create_default_wifi_sta();
    
    // 初始化WiFi
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
    // 注册事件处理器
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    
    // 设置WiFi模式
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    
    // 配置WiFi
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    
    strncpy((char*)wifi_config.sta.ssid, current_config.ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char*)wifi_config.sta.password, current_config.password, sizeof(wifi_config.sta.password));
    
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    ESP_LOGI(TAG, "WiFi初始化完成，正在连接...");
}
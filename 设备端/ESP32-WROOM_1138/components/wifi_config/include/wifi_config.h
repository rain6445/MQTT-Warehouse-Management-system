#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

// 用户自定义WiFi配置结构体，避免与ESP-IDF冲突
typedef struct {
    char ssid[32];
    char password[64];
    char city[32];
} user_wifi_config_t;

// 函数声明
void wifi_config_init(void);
void wifi_set_config(const char *ssid, const char *password);
void wifi_get_config(user_wifi_config_t *config);
void wifi_save_config(void);
void wifi_read_config(void);

#endif // WIFI_CONFIG_H
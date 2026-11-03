#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "driver/gpio.h"
#include "cJSON.h"

#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"

#include "wifi_config.h"
#include "mqtt_component.h"
#include "pn532_component.h"

static const char *TAG = "MAIN";

/* ------------------------------------------------------------------ */
/* 配置项                                                              */
/* ------------------------------------------------------------------ */
#define STUDENT_ID           "2340711138"
#define MQTT_BROKER_URL      "mqtt://103.236.64.247:1883"
#define MQTT_SUB_TOPIC       "1138_MQTT_Sub"
#define MQTT_DEV_TOPIC       "1138_Device"
#define MQTT_DEV_RESP_TOPIC  "1138_Device_Resp"

/* 原 LED：高电平点亮，GPIO13 */
#define LED_PIN         GPIO_NUM_13
#define LED_ON()        gpio_set_level(LED_PIN, 1)
#define LED_OFF()       gpio_set_level(LED_PIN, 0)

/* 绿色 LED：高电平点亮，GPIO26 (MQTT已连接 且 在库状态 时亮起) */
#define GREEN_LED_PIN   GPIO_NUM_26
#define GREEN_LED_ON()  gpio_set_level(GREEN_LED_PIN, 1)
#define GREEN_LED_OFF() gpio_set_level(GREEN_LED_PIN, 0)

/* 蜂鸣器：低电平触发，GPIO27 */
#define BUZZ_PIN        GPIO_NUM_27
#define BUZZ_ON()       gpio_set_level(BUZZ_PIN, 0)
#define BUZZ_OFF()      gpio_set_level(BUZZ_PIN, 1)

/* false = 设备已出库，暂停心跳；true = 设备在库，正常心跳 */
static volatile bool heartbeat_enabled = true;

/* 运行时生成，格式：ESP_AABBCC */
static char MQTT_CLIENT_ID[16];


/* ------------------------------------------------------------------ */
/* RSA 公钥                                                            */
/* ------------------------------------------------------------------ */
static const char RSA_PUBLIC_KEY[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAw+4pYJ1w5LJ4V8ZlU2o6\n"
    "bizh7hPhv9e1n35a0ws//Ykjp1XvZgOBKZ2FOB+yJATCMByqHi9w4j4dt3iRrNrF\n"
    "zkxuLRVX3jzpqQKk+EwlQ2b7CZBMvpM0NRdGibglVAdkidwYUP7Hu4fqTJ24IXSr\n"
    "lMxZoenOLr+7Ui2T8mfp6Dcm8wFI7bL8mK1MyvRMMvQXwraR0GlTzsFAJSCrSyhT\n"
    "FB6b86Sjz5zUB9jTfQ6xj2jV5IM8nKQvZlzZgvUxPLYlG/trv9W4KAt9f2jrvJMX\n"
    "Q0OoOL4oRdAn3Iipd5+H5ZmII/iulbpsyr2N63xTIu8RvM4dCNIzRP36KmJkKbaW\n"
    "WwIDAQAB\n"
    "-----END PUBLIC KEY-----\n";


/* ------------------------------------------------------------------ */
/* NVS 读写函数（用于断电记忆状态）                                     */
/* ------------------------------------------------------------------ */
static void save_heartbeat_state_to_nvs(bool state)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_u8(my_handle, "hb_state", (uint8_t)state);
        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "[NVS] 状态已保存: %s", state ? "在库(开启心跳)" : "出库(暂停心跳)");
    } else {
        ESP_LOGE(TAG, "[NVS] 打开失败，无法保存状态");
    }
}

static void load_heartbeat_state_from_nvs(void)
{
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_OK) {
        uint8_t state = 1;
        err = nvs_get_u8(my_handle, "hb_state", &state);
        if (err == ESP_OK) {
            heartbeat_enabled = (state != 0);
            ESP_LOGI(TAG, "[NVS] 成功读取历史状态: %s", heartbeat_enabled ? "在库" : "出库");
        } else if (err == ESP_ERR_NVS_NOT_FOUND) {
            ESP_LOGI(TAG, "[NVS] 未找到历史状态，使用默认值：在库");
            heartbeat_enabled = true;
        }
        nvs_close(my_handle);
    } else {
        ESP_LOGW(TAG, "[NVS] 打开失败，使用默认值：在库");
        heartbeat_enabled = true;
    }
}


/* ------------------------------------------------------------------ */
/* 用 MAC 后 3 字节生成唯一客户端 ID                                    */
/* ------------------------------------------------------------------ */
static void generate_client_id(void)
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    snprintf(MQTT_CLIENT_ID, sizeof(MQTT_CLIENT_ID),
             "ESP_%02X%02X%02X", mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "MQTT Client ID: %s", MQTT_CLIENT_ID);
}


/* ------------------------------------------------------------------ */
/* RSA-OAEP(SHA-256) 加密 → Base64                                    */
/* ------------------------------------------------------------------ */
static int rsa_encrypt_to_base64(const char *plaintext,
                                  char       *out_buf,
                                  size_t      out_buf_size)
{
    int ret = 0;

    mbedtls_pk_context       pk;
    mbedtls_entropy_context  entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    unsigned char encrypted[256];
    size_t        enc_len = 0;

    mbedtls_pk_init(&pk);
    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg,
                                 mbedtls_entropy_func, &entropy,
                                 (const unsigned char *)"esp32_rsa", 9);
    if (ret != 0) {
        ESP_LOGE(TAG, "ctr_drbg_seed 失败: -0x%04X", -ret);
        goto cleanup;
    }

    ret = mbedtls_pk_parse_public_key(&pk,
                                       (const unsigned char *)RSA_PUBLIC_KEY,
                                       sizeof(RSA_PUBLIC_KEY));
    if (ret != 0) {
        ESP_LOGE(TAG, "公钥解析失败: -0x%04X", -ret);
        goto cleanup;
    }

    mbedtls_rsa_set_padding(mbedtls_pk_rsa(pk),
                             MBEDTLS_RSA_PKCS_V21,
                             MBEDTLS_MD_SHA256);

    ret = mbedtls_pk_encrypt(&pk,
                              (const unsigned char *)plaintext,
                              strlen(plaintext),
                              encrypted, &enc_len, sizeof(encrypted),
                              mbedtls_ctr_drbg_random, &ctr_drbg);
    if (ret != 0) {
        ESP_LOGE(TAG, "RSA 加密失败: -0x%04X", -ret);
        goto cleanup;
    }

    size_t base64_len = 0;
    ret = mbedtls_base64_encode((unsigned char *)out_buf, out_buf_size,
                                 &base64_len, encrypted, enc_len);
    if (ret != 0) {
        ESP_LOGE(TAG, "Base64 编码失败，缓冲区不够大");
        ret = -1;
        goto cleanup;
    }

    ret = 0;

cleanup:
    mbedtls_pk_free(&pk);
    mbedtls_entropy_free(&entropy);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
}


/* ------------------------------------------------------------------ */
/* WiFi 事件回调：仅在库状态下，断开WiFi时报警                          */
/* ------------------------------------------------------------------ */
static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                                int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        /* 只有在库状态才蜂鸣报警 */
        if (heartbeat_enabled) {
            ESP_LOGW(TAG, "WiFi 断开（在库），蜂鸣器启动");
            BUZZ_ON();
        } else {
            ESP_LOGW(TAG, "WiFi 断开（出库），不报警");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "WiFi 已连接，蜂鸣器静音");
        BUZZ_OFF();
    }
}


/* ------------------------------------------------------------------ */
/* 蜂鸣器鸣叫函数                                                      */
/* ------------------------------------------------------------------ */
static void buzz_beep_once(void)
{
    BUZZ_ON();
    vTaskDelay(pdMS_TO_TICKS(200));
    BUZZ_OFF();
}

static void buzz_beep_twice(void)
{
    BUZZ_ON();
    vTaskDelay(pdMS_TO_TICKS(200));
    BUZZ_OFF();
    vTaskDelay(pdMS_TO_TICKS(150));
    BUZZ_ON();
    vTaskDelay(pdMS_TO_TICKS(200));
    BUZZ_OFF();
}

static void buzz_beep_long(void)
{
    BUZZ_ON();
    vTaskDelay(pdMS_TO_TICKS(1000));
    BUZZ_OFF();
}

static void buzz_task_once(void *arg)  { buzz_beep_once();  vTaskDelete(NULL); }
static void buzz_task_twice(void *arg) { buzz_beep_twice(); vTaskDelete(NULL); }
static void buzz_task_long(void *arg)  { buzz_beep_long();  vTaskDelete(NULL); }


/* ------------------------------------------------------------------ */
/* MQTT 订阅消息回调                                                   */
/* ------------------------------------------------------------------ */
static void mqtt_data_handler(const char *topic, const char *data,
                               int topic_len,      int data_len)
{
    ESP_LOGI(TAG, "收到 MQTT 消息");
    ESP_LOGI(TAG, "Topic: %.*s", topic_len, topic);
    ESP_LOGI(TAG, "Data : %.*s", data_len,  data);

    bool is_resp = (topic_len == (int)strlen(MQTT_DEV_RESP_TOPIC)) &&
                   (strncmp(topic, MQTT_DEV_RESP_TOPIC, topic_len) == 0);

    if (!is_resp) {
        return;
    }

    char *json_str = strndup(data, data_len);
    if (json_str == NULL) {
        ESP_LOGE(TAG, "[RESP] 内存不足");
        return;
    }

    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (root == NULL) {
        ESP_LOGE(TAG, "[RESP] JSON 解析失败");
        return;
    }

    cJSON *j_action = cJSON_GetObjectItem(root, "action");
    cJSON *j_passed = cJSON_GetObjectItem(root, "passed");
    cJSON *j_reason = cJSON_GetObjectItem(root, "reason");
    cJSON *j_swiper = cJSON_GetObjectItem(root, "swiper");

    const char *action = cJSON_IsString(j_action) ? j_action->valuestring : "none";
    bool        passed = cJSON_IsBool(j_passed)   ? cJSON_IsTrue(j_passed) : false;
    const char *reason = cJSON_IsString(j_reason) ? j_reason->valuestring : "";
    const char *swiper = cJSON_IsString(j_swiper) ? j_swiper->valuestring : "未知";

    ESP_LOGI(TAG, "[RESP] action=%s passed=%s swiper=%s reason=%s",
             action, passed ? "true" : "false", swiper, reason);

    if (strcmp(action, "none") == 0) {
        ESP_LOGW(TAG, "[RESP] 无效卡或未知设备");
        xTaskCreate(buzz_task_long, "buzz_long", 2048, NULL, 3, NULL);

    } else if (passed) {
        if (strcmp(action, "checkout") == 0) {
            ESP_LOGI(TAG, "[RESP] 出库成功，暂停心跳");
            heartbeat_enabled = false;
            save_heartbeat_state_to_nvs(false);
            /* 出库后立即熄灭绿色 LED */
            GREEN_LED_OFF();
        } else if (strcmp(action, "checkin") == 0) {
            ESP_LOGI(TAG, "[RESP] 入库成功，恢复心跳");
            heartbeat_enabled = true;
            save_heartbeat_state_to_nvs(true);
            /* 入库后立即点亮绿色 LED（前提是MQTT还在连接中） */
            if (mqtt_component_is_connected()) {
                GREEN_LED_ON();
            }
        }
        xTaskCreate(buzz_task_twice, "buzz_twice", 2048, NULL, 3, NULL);

    } else {
        ESP_LOGW(TAG, "[RESP] 有效卡，拒绝: %s", reason);
        xTaskCreate(buzz_task_once, "buzz_once", 2048, NULL, 3, NULL);
    }

    cJSON_Delete(root);
}


/* ------------------------------------------------------------------ */
/* 等待 WiFi 获取 IP                                                   */
/* ------------------------------------------------------------------ */
static void wait_wifi_connected(void)
{
    while (1) {
        esp_netif_t *sta = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
        if (sta) {
            esp_netif_ip_info_t ip;
            if (esp_netif_get_ip_info(sta, &ip) == ESP_OK && ip.ip.addr != 0) {
                ESP_LOGI(TAG, "WiFi 已连接，IP: " IPSTR, IP2STR(&ip.ip));
                return;
            }
        }
        ESP_LOGI(TAG, "等待 WiFi...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/* ------------------------------------------------------------------ */
/* MQTT 订阅任务：控制绿色 LED + 自动重订阅                             */
/* ------------------------------------------------------------------ */
static void mqtt_subscribe_task(void *arg)
{
    bool was_connected = false;

    while (1) {
        bool now_connected = mqtt_component_is_connected();

        /* 绿色 LED：MQTT已连接 且 设备在库 → 亮；否则 → 灭 */
        if (now_connected && heartbeat_enabled) {
            GREEN_LED_ON();
        } else {
            GREEN_LED_OFF();
        }

        /* 检测到从断开→连接的跳变时重新订阅 */
        if (now_connected && !was_connected) {
            ESP_LOGI(TAG, "[SUB] MQTT 已连接，订阅主题...");
            mqtt_component_subscribe(MQTT_SUB_TOPIC,      1);
            mqtt_component_subscribe(MQTT_DEV_RESP_TOPIC, 1);
            ESP_LOGI(TAG, "[SUB] 订阅完成");
        }

        was_connected = now_connected;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/* ------------------------------------------------------------------ */
/* 心跳任务                                                            */
/* ------------------------------------------------------------------ */
static void heartbeat_task(void *arg)
{
    ESP_LOGI(TAG, "[HB] 等待 MQTT 连接...");
    while (!mqtt_component_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
    }

    static char hb_b64[400];

    while (1) {
        if (!heartbeat_enabled) {
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (!mqtt_component_is_connected()) {
            ESP_LOGW(TAG, "[HB] MQTT 断开，暂停心跳");
            vTaskDelay(pdMS_TO_TICKS(3000));
            continue;
        }

        int64_t uptime_ms = esp_timer_get_time() / 1000;

        char json_str[128];
        snprintf(json_str, sizeof(json_str),
                 "{\"student_id\":\"%s\",\"device\":\"%s\","
                 "\"type\":\"heartbeat\",\"uptime_ms\":%lld}",
                 STUDENT_ID, MQTT_CLIENT_ID, (long long)uptime_ms);

        int enc_result = rsa_encrypt_to_base64(json_str, hb_b64, sizeof(hb_b64));
        if (enc_result == 0) {
            esp_err_t pub_err = mqtt_component_publish(
                MQTT_DEV_TOPIC, hb_b64, 1, 0);

            if (pub_err == ESP_OK) {
                ESP_LOGI(TAG, "[HB] 心跳已发布 (uptime=%llds)",
                         (long long)(uptime_ms / 1000));
                LED_ON();
                vTaskDelay(pdMS_TO_TICKS(100));
                LED_OFF();
            } else {
                ESP_LOGE(TAG, "[HB] 发布失败: %s", esp_err_to_name(pub_err));
            }
        } else {
            ESP_LOGE(TAG, "[HB] 加密失败，跳过本次心跳");
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}


/* ------------------------------------------------------------------ */
/* NFC 读卡任务                                                        */
/* ------------------------------------------------------------------ */
static void nfc_mqtt_task(void *arg)
{
    ESP_LOGI(TAG, "[NFC] 等待 MQTT 连接...");
    int wait_count = 0;
    while (!mqtt_component_is_connected()) {
        vTaskDelay(pdMS_TO_TICKS(500));
        if (++wait_count > 60) {
            ESP_LOGE(TAG, "[NFC] 等待 MQTT 超时，任务退出");
            vTaskDelete(NULL);
            return;
        }
    }

    esp_err_t err = pn532_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "[NFC] PN532 初始化失败 (%s)", esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "[NFC] PN532 就绪，开始轮询...");

    char       last_uid[32]   = {0};
    TickType_t last_read_tick = 0;
    static char nfc_b64[400];

    while (1) {
        if (!mqtt_component_is_connected()) {
            ESP_LOGW(TAG, "[NFC] MQTT 断开，暂停读卡");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        char *uid = pn532_read_card();

        if (uid == NULL) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        TickType_t now   = xTaskGetTickCount();
        bool same_card   = (strcmp(uid, last_uid) == 0);
        bool in_cooldown = ((now - last_read_tick) < pdMS_TO_TICKS(3000));

        if (same_card && in_cooldown) {
            vTaskDelay(pdMS_TO_TICKS(200));
            continue;
        }

        strncpy(last_uid, uid, sizeof(last_uid) - 1);
        last_read_tick = now;

        ESP_LOGI(TAG, "[NFC] 卡片 UID: %s", uid);

        cJSON *root = cJSON_CreateObject();
        if (root == NULL) {
            ESP_LOGE(TAG, "[NFC] cJSON 创建失败");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        cJSON_AddStringToObject(root, "student_id", STUDENT_ID);
        cJSON_AddStringToObject(root, "device",     MQTT_CLIENT_ID);
        cJSON_AddStringToObject(root, "type",       "nfc");
        cJSON_AddStringToObject(root, "uid",        uid);

        char *json_str = cJSON_PrintUnformatted(root);
        cJSON_Delete(root);

        if (json_str == NULL) {
            ESP_LOGE(TAG, "[NFC] JSON 序列化失败");
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        ESP_LOGI(TAG, "[NFC] 明文: %s", json_str);

        int enc_result = rsa_encrypt_to_base64(json_str, nfc_b64, sizeof(nfc_b64));
        if (enc_result == 0) {
            esp_err_t pub_err = mqtt_component_publish(
                MQTT_DEV_TOPIC, nfc_b64, 1, 0);

            if (pub_err == ESP_OK)
                ESP_LOGI(TAG, "[NFC] 已发布至 %s", MQTT_DEV_TOPIC);
            else
                ESP_LOGE(TAG, "[NFC] 发布失败: %s", esp_err_to_name(pub_err));
        } else {
            ESP_LOGE(TAG, "[NFC] 加密失败，跳过");
        }

        cJSON_free(json_str);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}


/* ------------------------------------------------------------------ */
/* app_main                                                            */
/* ------------------------------------------------------------------ */
void app_main(void)
{
    /* 1. NVS 初始化 */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    /* 开机自动从 NVS 读取历史出入库状态 */
    load_heartbeat_state_from_nvs();

    /* 2. 网络基础 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 3. 生成唯一客户端 ID */
    generate_client_id();

    /* 4. LED 初始化（高电平点亮，默认熄灭） */
    gpio_config_t led_conf = {
        .pin_bit_mask = (1ULL << LED_PIN) | (1ULL << GREEN_LED_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&led_conf);
    LED_OFF();
    GREEN_LED_OFF();

    /* 5. 蜂鸣器初始化（低电平触发，默认静音） */
    gpio_config_t buzz_conf = {
        .pin_bit_mask = (1ULL << BUZZ_PIN),
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&buzz_conf);
    BUZZ_OFF();

    /* 6. 注册 WiFi 事件 */
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED,
        &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP,
        &wifi_event_handler, NULL));

    /* 7. WiFi */
    wifi_config_init();
    wait_wifi_connected();

    /* 8. MQTT */
    mqtt_config_t mqtt_cfg = {
        .student_id = STUDENT_ID,
        .broker_url = MQTT_BROKER_URL,
    };
    strncpy(mqtt_cfg.client_id,
            MQTT_CLIENT_ID,
            sizeof(mqtt_cfg.client_id) - 1);
    mqtt_cfg.client_id[sizeof(mqtt_cfg.client_id) - 1] = '\0';

    ESP_ERROR_CHECK(mqtt_component_init(&mqtt_cfg));
    mqtt_component_set_callback(mqtt_data_handler);
    ESP_ERROR_CHECK(mqtt_component_start());

    /* 9. 启动任务 */
    xTaskCreate(mqtt_subscribe_task, "mqtt_sub_task",  4096, NULL, 6, NULL);
    xTaskCreate(heartbeat_task,      "heartbeat_task", 8192, NULL, 4, NULL);
    xTaskCreate(nfc_mqtt_task,       "nfc_mqtt_task",  8192, NULL, 5, NULL);

    ESP_LOGI(TAG, "系统就绪");
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

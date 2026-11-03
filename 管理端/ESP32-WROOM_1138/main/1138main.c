#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "cJSON.h"

#include "mbedtls/pk.h"
#include "mbedtls/rsa.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"

#include "wifi_config.h"
#include "mqtt_component.h"
#include "pn532_component.h"
#include "dht11_component.h"
#include "oled.h"

static const char *TAG = "MAIN";

#define STUDENT_ID      "2340711138"
#define MQTT_BROKER_URL "mqtt://103.236.64.247:1883"
#define MQTT_CLIENT_ID  "1138_ESP32"

#define MQTT_PUB_TOPIC  "1138_MQTT_Pub_JSON"
#define MQTT_SUB_TOPIC  "1138_MQTT_Sub"
#define MQTT_NFC_TOPIC  "1138_NFC_Card"


/* ------------------------------------------------------------------ */
/*  RSA-2048 公钥                                                       */
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


/* ================================================================== */
/*  OLED 布局常量                                                       */
/*                                                                      */
/*  字体 16px，每行高 16px = 2 页（1 页 = 8px）                        */
/*                                                                      */
/*   y=0   pages 0-1 : "ID:  2340711138"   静态，只画一次              */
/*   y=16  pages 2-3 : "Temp:  XX.X C"     标签静态，数值动态          */
/*   y=32  pages 4-5 : "Humi:  XX.X %"     标签静态，数值动态          */
/*   y=48  pages 6-7 : 状态行              WiFi/MQTT 异常时显示        */
/* ================================================================== */
#define OLED_FONT        16
#define OLED_CHAR_W       8     /* 16px 字体每字符宽 8px */

#define ROW_ID_Y          0
#define ROW_TEMP_Y       16
#define ROW_HUMI_Y       32
#define ROW_STATUS_Y     48

#define ROW_ID_PAGE_S     0
#define ROW_ID_PAGE_E     1
#define ROW_TEMP_PAGE_S   2
#define ROW_TEMP_PAGE_E   3
#define ROW_HUMI_PAGE_S   4
#define ROW_HUMI_PAGE_E   5
#define ROW_STAT_PAGE_S   6
#define ROW_STAT_PAGE_E   7

/* 各标签字符数 × 字宽 = 数值区域起始 x */
#define LABEL_TEMP_W     (6 * OLED_CHAR_W)   /* "Temp: " = 48px */
#define LABEL_HUMI_W     (6 * OLED_CHAR_W)   /* "Humi: " = 48px */


/* ------------------------------------------------------------------ */
/*  OLED 布局函数                                                       */
/* ------------------------------------------------------------------ */

/**
 * @brief 初始化静态布局，只在启动时调用一次
 *        将不变的标签写入 GRAM 并全屏刷新
 */
static void oled_init_layout(void)
{
    OLED_Clear();   /* 全清 + 全刷，整个生命周期只做这一次 */

    OLED_ShowString(0, ROW_ID_Y,   "ID:" STUDENT_ID, OLED_FONT, 1);
    OLED_ShowString(0, ROW_TEMP_Y, "Temp: ",          OLED_FONT, 1);
    OLED_ShowString(0, ROW_HUMI_Y, "Humi: ",          OLED_FONT, 1);

    OLED_Refresh(); /* 把三行标签推到屏幕 */
}


/**
 * @brief 只更新 Temp / Humi 数值区域
 *        流程：清除数值列 GRAM → 写新字符到 GRAM → 仅刷新 pages 2-5
 *
 * @param temperature  温度（℃），dht_ok=false 时忽略
 * @param humidity     湿度（%），dht_ok=false 时忽略
 * @param dht_ok       DHT11 读取是否成功
 */
static void oled_update_sensor(float temperature, float humidity, bool dht_ok)
{
    char val[12];

    /* Temp 行 */
    OLED_ClearArea(LABEL_TEMP_W, 127, ROW_TEMP_PAGE_S, ROW_TEMP_PAGE_E);
    if (dht_ok)
        snprintf(val, sizeof(val), "%5.1fC", temperature);
    else
        snprintf(val, sizeof(val), " -- C");
    OLED_ShowString(LABEL_TEMP_W, ROW_TEMP_Y, val, OLED_FONT, 1);

    /* Humi 行 */
    OLED_ClearArea(LABEL_HUMI_W, 127, ROW_HUMI_PAGE_S, ROW_HUMI_PAGE_E);
    if (dht_ok)
        snprintf(val, sizeof(val), "%5.1f%%", humidity);
    else
        snprintf(val, sizeof(val), " -- %%");
    OLED_ShowString(LABEL_HUMI_W, ROW_HUMI_Y, val, OLED_FONT, 1);

    /* 只刷新 Temp + Humi 所在的 pages 2-5，其余行不动 */
    OLED_RefreshPages(ROW_TEMP_PAGE_S, ROW_HUMI_PAGE_E);
}


/**
 * @brief 更新最底行状态信息
 *        msg == NULL 时清空状态行（正常状态）
 *
 * @param msg  最多 16 个字符的提示字符串，或 NULL
 */
static void oled_set_status(const char *msg)
{
    OLED_ClearArea(0, 127, ROW_STAT_PAGE_S, ROW_STAT_PAGE_E);

    if (msg != NULL) {
        char buf[17];
        snprintf(buf, sizeof(buf), "%-16s", msg);
        OLED_ShowString(0, ROW_STATUS_Y, buf, OLED_FONT, 1);
    }

    OLED_RefreshPages(ROW_STAT_PAGE_S, ROW_STAT_PAGE_E);
}


/* ------------------------------------------------------------------ */
/*  RSA 加密                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief 用 RSA-OAEP(SHA-256) 加密数据，输出 Base64 字符串
 *
 * @param plaintext     明文字符串，长度不得超过 ~190 字节
 * @param out_buf       输出缓冲区（建议 400 字节）
 * @param out_buf_size  输出缓冲区大小
 * @return  0 成功，-1 失败
 */
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
                                 mbedtls_entropy_func,
                                 &entropy,
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
        ESP_LOGE(TAG, "Base64 编码失败，缓冲区可能不够大");
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
/*  MQTT 回调                                                           */
/* ------------------------------------------------------------------ */

static void mqtt_data_handler(const char *topic,
                              const char *data,
                              int topic_len,
                              int data_len)
{
    ESP_LOGI(TAG, "收到 MQTT 消息");
    ESP_LOGI(TAG, "Topic: %.*s", topic_len, topic);
    ESP_LOGI(TAG, "Data : %.*s", data_len, data);
}


/* ------------------------------------------------------------------ */
/*  WiFi 等待                                                           */
/* ------------------------------------------------------------------ */

static void wait_wifi_connected(void)
{
    while (1) {
        esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");

        if (sta_netif) {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK &&
                ip_info.ip.addr != 0) {
                ESP_LOGI(TAG, "WiFi 已连接，IP: " IPSTR, IP2STR(&ip_info.ip));
                return;
            }
        }

        ESP_LOGI(TAG, "等待 WiFi 连接...");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


/* ------------------------------------------------------------------ */
/*  DHT11 发布任务                                                      */
/* ------------------------------------------------------------------ */

/**
 * @brief DHT11 温湿度加密发布任务
 *
 * 每 5 秒读取一次 DHT11，将 JSON 用 RSA-OAEP 公钥加密后发布到
 * MQTT_PUB_TOPIC，同时只刷新 OLED 的温湿度数值区域。
 * WiFi/MQTT 断开时在底行显示状态提示。
 */
static void mqtt_json_publish_task(void *arg)
{
    uint32_t seq        = 0;
    bool     subscribed = false;
    bool     was_online = false;   /* 上一轮是否在线，用于检测状态变化 */

    static char encrypted_b64[400];

    while (1) {
        if (mqtt_component_is_connected()) {

            /* 刚从离线恢复 → 清除状态行 */
            if (!was_online) {
                oled_set_status(NULL);
                was_online = true;
            }

            /* MQTT 重连后重新订阅 */
            if (!subscribed) {
                esp_err_t sub_err = mqtt_component_subscribe(MQTT_SUB_TOPIC, 1);
                if (sub_err == ESP_OK) {
                    subscribed = true;
                    ESP_LOGI(TAG, "已订阅主题: %s", MQTT_SUB_TOPIC);
                } else {
                    ESP_LOGW(TAG, "订阅失败: %s", esp_err_to_name(sub_err));
                }
            }

            /* 读取 DHT11 */
            float temperature = 0.0f;
            float humidity    = 0.0f;
            int   dht_result  = dht11_read(&temperature, &humidity);
            bool  dht_ok      = (dht_result == 0);

            if (!dht_ok)
                ESP_LOGW(TAG, "DHT11 读取失败");

            /* 只刷新温湿度数值区域，其余行不变 */
            oled_update_sensor(temperature, humidity, dht_ok);

            /* 构造 JSON */
            int64_t uptime_ms = esp_timer_get_time() / 1000;
            char    json_data[192];

            if (dht_ok) {
                snprintf(json_data, sizeof(json_data),
                         "{\"student_id\":\"%s\",\"seq\":%lu,"
                         "\"uptime_ms\":%lld,\"temp\":%.1f,\"humi\":%.1f}",
                         STUDENT_ID,
                         (unsigned long)seq,
                         (long long)uptime_ms,
                         temperature,
                         humidity);
            } else {
                snprintf(json_data, sizeof(json_data),
                         "{\"student_id\":\"%s\",\"seq\":%lu,"
                         "\"uptime_ms\":%lld,\"error\":\"dht11_read_failed\"}",
                         STUDENT_ID,
                         (unsigned long)seq,
                         (long long)uptime_ms);
            }

            /* 加密并发布 */
            int enc_result = rsa_encrypt_to_base64(json_data,
                                                    encrypted_b64,
                                                    sizeof(encrypted_b64));
            if (enc_result == 0) {
                esp_err_t pub_err = mqtt_component_publish(MQTT_PUB_TOPIC,
                                                            encrypted_b64,
                                                            1, 0);
                if (pub_err == ESP_OK)
                    ESP_LOGI(TAG, "已发布加密消息 (seq=%lu)", (unsigned long)seq);
                else
                    ESP_LOGW(TAG, "MQTT 发布失败: %s", esp_err_to_name(pub_err));
            } else {
                ESP_LOGE(TAG, "加密失败，跳过发布");
            }

            seq++;

        } else {
            /* MQTT 断开 */
            subscribed = false;
            was_online = false;
            ESP_LOGW(TAG, "MQTT 未连接，跳过本次发布");
            oled_set_status("MQTT offline");
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}


/* ------------------------------------------------------------------ */
/*  NFC 任务                                                            */
/* ------------------------------------------------------------------ */

/**
 * @brief NFC 读卡并发布 MQTT 任务
 *
 * 读到卡后将 UID 以 JSON 加密发布到 MQTT_NFC_TOPIC。
 * 同一张卡 3 秒内只上报一次。
 */
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
        ESP_LOGE(TAG, "[NFC] PN532 初始化失败 (%s)，任务退出",
                 esp_err_to_name(err));
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "[NFC] PN532 就绪，开始轮询卡片...");

    char       last_uid[32]   = {0};
    TickType_t last_read_tick = 0;
    static char nfc_b64[400];

    while (1) {
        if (!mqtt_component_is_connected()) {
            ESP_LOGW(TAG, "[NFC] MQTT 已断开，暂停读卡");
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }

        char *uid = pn532_read_card();

        if (uid != NULL) {
            TickType_t now = xTaskGetTickCount();

            bool same_card   = (strcmp(uid, last_uid) == 0);
            bool in_cooldown = ((now - last_read_tick) < pdMS_TO_TICKS(3000));

            if (same_card && in_cooldown) {
                vTaskDelay(pdMS_TO_TICKS(200));
                continue;
            }

            strncpy(last_uid, uid, sizeof(last_uid) - 1);
            last_read_tick = now;

            ESP_LOGI(TAG, "[NFC] 检测到卡片 UID: %s", uid);

            /* 底行短暂显示卡号 */
            char uid_line[17];
            snprintf(uid_line, sizeof(uid_line), "NFC:%.11s", uid);
            oled_set_status(uid_line);

            /* 构造 JSON */
            cJSON *root = cJSON_CreateObject();
            if (root == NULL) {
                ESP_LOGE(TAG, "[NFC] cJSON 创建对象失败");
                vTaskDelay(pdMS_TO_TICKS(1000));
                continue;
            }

            cJSON_AddStringToObject(root, "student_id", STUDENT_ID);
            cJSON_AddStringToObject(root, "uid",        uid);
            cJSON_AddStringToObject(root, "device",     "esp32_nfc");

            char *json_str = cJSON_PrintUnformatted(root);
            cJSON_Delete(root);

            if (json_str != NULL) {
                int enc_result = rsa_encrypt_to_base64(json_str,
                                                        nfc_b64,
                                                        sizeof(nfc_b64));
                if (enc_result == 0) {
                    esp_err_t pub_err = mqtt_component_publish(
                        MQTT_NFC_TOPIC, nfc_b64, 1, 0);

                    if (pub_err == ESP_OK)
                        ESP_LOGI(TAG, "[NFC] 已发布加密卡号");
                    else
                        ESP_LOGE(TAG, "[NFC] MQTT 发布失败: %s",
                                 esp_err_to_name(pub_err));
                } else {
                    ESP_LOGE(TAG, "[NFC] 加密失败，跳过发布");
                }

                cJSON_free(json_str);
            }

            /* 卡号显示 1.5 秒后由下次温湿度刷新或状态更新覆盖 */
            vTaskDelay(pdMS_TO_TICKS(1500));

            /* 恢复底行为空（如果 MQTT 正常） */
            if (mqtt_component_is_connected())
                oled_set_status(NULL);

        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }
    }
}


/* ------------------------------------------------------------------ */
/*  app_main                                                            */
/* ------------------------------------------------------------------ */

void app_main(void)
{
    /* 1. 初始化 NVS */
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_LOGI(TAG, "系统启动");

    /* 2. 初始化网络栈 */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* 3. 初始化 WiFi */
    wifi_config_init();

    /* 4. 初始化 DHT11 */
    dht11_init();

    /* 5. 初始化 OLED，画静态布局，底行显示等待状态 */
    OLED_Init();
    oled_init_layout();
    oled_set_status("WiFi connecting");

    /* 6. 等待 WiFi 获取 IP */
    wait_wifi_connected();
    oled_set_status("MQTT starting..");

    /* 7. 启动 MQTT */
    mqtt_config_t mqtt_cfg = {
        .student_id = STUDENT_ID,
        .broker_url = MQTT_BROKER_URL,
        .client_id  = MQTT_CLIENT_ID,
    };
    ESP_ERROR_CHECK(mqtt_component_init(&mqtt_cfg));
    mqtt_component_set_callback(mqtt_data_handler);
    ESP_ERROR_CHECK(mqtt_component_start());

    /* 8. 创建任务 */
    xTaskCreate(mqtt_json_publish_task, "mqtt_pub_task", 8192, NULL, 5, NULL);
    xTaskCreate(nfc_mqtt_task,          "nfc_mqtt_task", 8192, NULL, 4, NULL);

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

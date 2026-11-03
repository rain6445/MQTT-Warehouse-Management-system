#include "pn532_component.h"
#include <string.h>
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "PN532";

// 静态缓冲区存储 UID 字符串，最多 10 字节 UID × 2 字符 + 结尾符
static char uid_str_buffer[32];

esp_err_t pn532_init(void)
{
    uart_config_t uart_config = {
        .baud_rate  = 115200,
        .data_bits  = UART_DATA_8_BITS,
        .parity     = UART_PARITY_DISABLE,
        .stop_bits  = UART_STOP_BITS_1,
        .flow_ctrl  = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    esp_err_t err;

    err = uart_param_config(PN532_UART_PORT, &uart_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_param_config 失败: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_set_pin(PN532_UART_PORT, PN532_TXD, PN532_RXD,
                       UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_set_pin 失败: %s", esp_err_to_name(err));
        return err;
    }

    err = uart_driver_install(PN532_UART_PORT, 512, 0, 0, NULL, 0);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "uart_driver_install 失败: %s", esp_err_to_name(err));
        return err;
    }

    // 等待 PN532 上电稳定
    vTaskDelay(pdMS_TO_TICKS(500));

    // SAMConfiguration 指令
    // 前面 0x55 0x55 0x00... 是串口唤醒序列
    // 0xD4 0x14 0x01 是 SAMConfiguration：普通模式，不使用 IRQ
    const uint8_t sam_cmd[] = {
        0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4,
        0x14, 0x01, 0x17, 0x00
    };
    uart_write_bytes(PN532_UART_PORT, sam_cmd, sizeof(sam_cmd));

    vTaskDelay(pdMS_TO_TICKS(100));

    // 清空 PN532 的 SAMConfiguration 应答，避免影响后续读卡
    uart_flush_input(PN532_UART_PORT);

    ESP_LOGI(TAG, "PN532 初始化完成 (TX=GPIO%d, RX=GPIO%d)", PN532_TXD, PN532_RXD);
    return ESP_OK;
}

char* pn532_read_card(void)
{
    // InListPassiveTarget：寻找 1 个 ISO14443A 卡
    const uint8_t read_cmd[] = {
        0x00, 0x00, 0xFF, 0x04, 0xFC,
        0xD4, 0x4A, 0x01, 0x00, 0xE1, 0x00
    };

    uart_flush_input(PN532_UART_PORT);
    uart_write_bytes(PN532_UART_PORT, read_cmd, sizeof(read_cmd));

    uint8_t rx_buffer[64];
    int len = uart_read_bytes(PN532_UART_PORT, rx_buffer,
                              sizeof(rx_buffer) - 1, pdMS_TO_TICKS(150));

    if (len <= 15) {
        return NULL; // 数据太短，肯定没卡
    }

    // 在返回帧中搜索: 0xD5 0x4B 0x01 表示找到 1 张卡
    for (int i = 0; i < len - 10; i++) {
        if (rx_buffer[i]   == 0xD5 &&
            rx_buffer[i+1] == 0x4B &&
            rx_buffer[i+2] == 0x01)
        {
            uint8_t uid_len = rx_buffer[i + 7];

            // 安全校验
            if (uid_len == 0 || uid_len > 10 || (i + 8 + uid_len) > len) {
                break;
            }

            memset(uid_str_buffer, 0, sizeof(uid_str_buffer));

            // 把每个 UID 字节转成 2 位大写十六进制
            for (int j = 0; j < uid_len; j++) {
                char hex[4];
                snprintf(hex, sizeof(hex), "%02X", rx_buffer[i + 8 + j]);
                strncat(uid_str_buffer, hex,
                        sizeof(uid_str_buffer) - strlen(uid_str_buffer) - 1);
            }

            ESP_LOGD(TAG, "读到 UID: %s (长度=%d)", uid_str_buffer, uid_len);
            return uid_str_buffer;
        }
    }

    return NULL;
}

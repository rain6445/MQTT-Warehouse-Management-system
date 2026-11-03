#ifndef PN532_COMPONENT_H
#define PN532_COMPONENT_H

#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// PN532 使用 UART2
// ESP32 GPIO17 (TX) → PN532 SCL (RX)
// ESP32 GPIO16 (RX) → PN532 SDA (TX)
#define PN532_UART_PORT UART_NUM_2
#define PN532_TXD       GPIO_NUM_17
#define PN532_RXD       GPIO_NUM_16

/**
 * @brief 初始化 PN532 模块（配置 UART、发送 SAMConfiguration）
 * @return ESP_OK 成功，其他值失败
 */
esp_err_t pn532_init(void);

/**
 * @brief 尝试读取一张卡的 UID
 * @return UID 字符串（如 "04A1B2C3"），没有卡时返回 NULL
 *         注意：返回的指针指向内部静态缓冲区，下次调用会覆盖
 */
char* pn532_read_card(void);

#ifdef __cplusplus
}
#endif

#endif // PN532_COMPONENT_H

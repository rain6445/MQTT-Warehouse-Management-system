#ifndef BT_SERVER_H
#define BT_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief 初始化 BLE 服务器
 * 
 * 初始化蓝牙并启动 BLE 服务，设置服务 UUID 和特征值
 * 创建广播，等待设备连接
 */
void bt_server_init(void);

/**
 * @brief 发送数据到已连接的设备
 * 
 * @param data 要发送的数据指针
 * @param length 数据长度
 * @return true 发送成功
 * @return false 发送失败（无连接）
 */
bool bt_server_send_data(const uint8_t *data, uint16_t length);

/**
 * @brief 检查蓝牙连接状态
 * 
 * @return true 已连接
 * @return false 未连接
 */
bool bt_server_is_connected(void);

/**
 * @brief 设置接收数据回调函数
 * 
 * @param callback 回调函数指针
 * 
 * 回调函数原型: void callback(uint8_t *data, uint16_t length)
 */
void bt_server_set_receive_callback(void (*callback)(uint8_t *, uint16_t));

#ifdef __cplusplus
}
#endif

#endif // BT_SERVER_H
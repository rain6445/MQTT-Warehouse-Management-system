#ifndef DHT11_COMPONENT_H
#define DHT11_COMPONENT_H

#include "driver/gpio.h"

// 定义你 DHT11 数据线连接的引脚
#define DHT11_PIN GPIO_NUM_4

// 初始化函数
void dht11_init(void);

// 读取温湿度，返回 0 表示成功，返回 -1 表示失败
int dht11_read(float *temperature, float *humidity);

#endif

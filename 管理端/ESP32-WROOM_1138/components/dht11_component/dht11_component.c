#include "dht11_component.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "rom/ets_sys.h" // 包含微秒级延时函数 ets_delay_us

static const char *TAG = "DHT11";

void dht11_init(void) {
    ESP_LOGI(TAG, "DHT11 已初始化, 引脚: %d", DHT11_PIN);
    // 这里其实不需要特别的初始化，因为单总线在读的时候会频繁切换输入输出状态
}

static int wait_for_state(int state, int timeout_us) {
    int time_waited = 0;
    while (gpio_get_level(DHT11_PIN) != state) {
        if (time_waited > timeout_us) return -1;
        ets_delay_us(1);
        time_waited++;
    }
    return time_waited;
}

int dht11_read(float *temperature, float *humidity) {
    uint8_t data[5] = {0};
    
    // 发送起始信号
    gpio_set_direction(DHT11_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(20)); // DHT11需要拉低至少18ms
    gpio_set_level(DHT11_PIN, 1);
    ets_delay_us(30);
    
    gpio_set_direction(DHT11_PIN, GPIO_MODE_INPUT);

    // 等待 DHT11 响应
    if (wait_for_state(0, 80) == -1) return -1;
    if (wait_for_state(1, 80) == -1) return -1;
    if (wait_for_state(0, 80) == -1) return -1;

    // 读取 40 bit 数据 (5 bytes)
    for (int i = 0; i < 40; i++) {
        if (wait_for_state(1, 80) == -1) return -1;
        int high_time = wait_for_state(0, 80);
        if (high_time == -1) return -1;
        
        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);
        
        // 如果高电平时间大于 40us，则认为是数字 1
        if (high_time > 40) {
            data[byte_idx] |= (1 << bit_idx);
        }
    }

    // 校验和
    if (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF)) {
        *humidity = data[0] + data[1] * 0.1;
        *temperature = data[2] + data[3] * 0.1;
        return 0; // 成功
    } else {
        ESP_LOGE(TAG, "DHT11 校验和错误!");
        return -1;
    }
}

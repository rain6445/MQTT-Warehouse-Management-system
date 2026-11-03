# MQTT客户端组件
idf_component_register(
    SRCS "mqtt_component.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt mbedtls esp_event esp_netif nvs_flash freertos
    
)
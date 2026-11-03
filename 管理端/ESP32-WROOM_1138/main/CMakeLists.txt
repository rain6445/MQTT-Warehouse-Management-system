set(requires esp-tls nvs_flash esp_netif esp_http_server mbedtls)
idf_build_get_property(target IDF_TARGET)

if(${target} STREQUAL "linux")
    list(APPEND requires esp_stubs protocol_examples_common)
else()
    list(APPEND requires esp_wifi esp_eth)
endif()

idf_component_register(
    SRCS "1138main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES ${requires}
    REQUIRES
        wifi_config
        mqtt_component
        pn532_component
        dht11_component
        json
        mbedtls
        oled_component
)

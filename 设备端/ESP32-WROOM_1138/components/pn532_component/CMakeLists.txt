idf_component_register(
    SRCS "pn532_component.c"
    INCLUDE_DIRS "include"
    REQUIRES
        driver
        freertos
        esp_common
        log
)

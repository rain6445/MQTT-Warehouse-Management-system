idf_component_register(
    SRCS "bt_server.c"
    INCLUDE_DIRS "include"
    REQUIRES bt nvs_flash driver esp_timer log
)

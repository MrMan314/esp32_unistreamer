idf_component_register(SRCS main.c gf256.c matrices.c
                        PRIV_INCLUDE_DIRS .
                        PRIV_REQUIRES nvs_flash esp_psram esp_wifi esp_event esp_timer)

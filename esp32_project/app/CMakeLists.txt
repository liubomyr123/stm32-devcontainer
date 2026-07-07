idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "."
    PRIV_REQUIRES 
        esp_wifi
        esp_event
        esp_http_server
        nvs_flash
        esp_netif
)
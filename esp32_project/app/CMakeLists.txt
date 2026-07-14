idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS "." "../esp"
    PRIV_REQUIRES 
        esp_wifi
        esp_event
        esp_http_server
        nvs_flash
        esp_netif
        esp32-camera
        lwip
        driver
)
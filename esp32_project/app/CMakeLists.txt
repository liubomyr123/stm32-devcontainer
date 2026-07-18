file(GLOB APP_SOURCES "src/*.cpp")

idf_component_register(
    SRCS 
        "app.cpp"
        ${APP_SOURCES}
    INCLUDE_DIRS 
        "."
        "includes"
        "../esp"
    PRIV_REQUIRES 
        esp_wifi
        esp_event
        esp_http_server
        nvs_flash
        esp_netif
        esp32-camera
        lwip
        driver
        fatfs
        sdmmc
)
#!/bin/bash

echo "==> Running clang-tidy..."

while IFS= read -r file; do
    clang-tidy "$file" \
        --config-file=/workspace/esp32_project/.clang-tidy \
        -- \
        -std=c++17 \
        -I/workspace/esp32_project/app \
        -I/opt/esp/idf/components/freertos/FreeRTOS-Kernel/include \
        -I/opt/esp/idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include \
        -I/opt/esp/idf/components/freertos/config/include/freertos \
        -I/opt/esp/idf/components/esp_common/include \
        -I/opt/esp/idf/components/esp_hw_support/include \
        -I/opt/esp/idf/components/log/include \
        -I/workspace/esp32_project/esp/build/config \
        -I/opt/esp/idf/components/esp_event/include \
        2>&1 | grep -A 3 "^/workspace/esp32_project/app" || true
done < <(find /workspace/esp32_project/app \( -name "*.cpp" -o -name "*.hpp" \))

echo "==> Done!"
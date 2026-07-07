#!/bin/bash

echo "==> Formatting app/ code..."

find /workspace/esp32_project/app -name "*.c" -o -name "*.h" | xargs clang-format -i

echo "==> Done!"

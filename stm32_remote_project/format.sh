#!/bin/bash

echo "==> Formatting app/ code..."

find /workspace/stm32_remote_project/app -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" | xargs clang-format -i

echo "==> Done!"

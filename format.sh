#!/bin/bash

echo "==> Formatting app/ code..."

find /workspace/app -name "*.cpp" -o -name "*.hpp" -o -name "*.h" -o -name "*.c" | xargs clang-format -i

echo "==> Done!"

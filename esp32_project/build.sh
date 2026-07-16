#!/bin/bash
set -e

cd /workspace/esp32_project/esp

# echo "==> Cleaning..."
# idf.py fullclean

echo "==> Building..."
idf.py build

echo "==> Done!"

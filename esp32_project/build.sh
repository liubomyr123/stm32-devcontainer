#!/bin/bash
set -e

cd /workspace/esp32_project/esp

echo "==> Building..."
idf.py build

echo "==> Done!"

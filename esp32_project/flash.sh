#!/bin/bash
set -e

cd /workspace/esp32_project/esp

echo "==> Flashing..."
idf.py -p /dev/ttyUSB0 -b 115200 flash

echo "==> Done!"

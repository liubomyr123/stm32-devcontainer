#!/bin/bash
set -e

cd /workspace/esp32_project/esp

echo "==> Flashing..."
idf.py -p /dev/ttyUSB0 flash

echo "==> Done!"

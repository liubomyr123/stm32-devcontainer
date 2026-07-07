#!/bin/bash
set -e

LOGS_DIR="/workspace/esp32_project/logs"
mkdir -p $LOGS_DIR

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOGS_DIR/session_$TIMESTAMP.log"

echo "==> Monitoring... (logging to $LOG_FILE)"

cd /workspace/esp32_project/esp
idf.py -p /dev/ttyUSB0 monitor | tee $LOG_FILE

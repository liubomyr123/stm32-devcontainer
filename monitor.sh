#!/bin/bash

echo "==> Setting permissions..."
sudo chmod 666 /dev/ttyACM0

LOGS_DIR="/workspace/logs"
mkdir -p $LOGS_DIR

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOGS_DIR/session_$TIMESTAMP.log"

echo "==> Starting monitor... (logging to $LOG_FILE)"
picocom -b 115200 /dev/ttyACM0 | tee $LOG_FILE

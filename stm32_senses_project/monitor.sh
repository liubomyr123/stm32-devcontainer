#!/bin/bash

echo "==> Looking for serial device..."
DEVICE=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n 1)

if [ -z "$DEVICE" ]; then
    echo "==> No serial device found. Connect FTDI and try again."
    echo "    Checked: /dev/ttyUSB*, /dev/ttyACM*"
    exit 1
fi

echo "==> Found device: $DEVICE"
echo "==> Setting permissions..."
sudo chmod 666 $DEVICE

LOGS_DIR="/workspace/stm32_senses_project/logs"
mkdir -p $LOGS_DIR

TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
LOG_FILE="$LOGS_DIR/session_$TIMESTAMP.log"

echo "==> Starting monitor... (logging to $LOG_FILE)"
picocom -b 115200 $DEVICE | tee $LOG_FILE
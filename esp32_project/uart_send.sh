#!/bin/bash

# Скрипт для надсилання команд на ESP32-CAM через UART
# ESP32-CAM GPIO13 (RX) ← FTDI TX
# ESP32-CAM GPIO12 (TX) → FTDI RX
# Використовується для тестування без STM32

PORT=${1:-/dev/ttyUSB1}
BAUD=${2:-115200}

echo "==> UART sender on $PORT at $BAUD baud"
echo "==> Press Ctrl+C to exit"

stty -F $PORT $BAUD cs8 -cstopb -parenb raw

# Слухаємо що приходить від ESP32-CAM у фоні
cat $PORT | while read line; do
    echo "<==: $line"
done &

while true; do
    read -p "> " cmd
    if [ -n "$cmd" ]; then
        printf "%s\n" "$cmd" > $PORT
        echo "==>: $cmd"
    fi
done
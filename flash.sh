#!/bin/bash

set -e

echo "==> Setting USB permissions..."
DEVICE=$(lsusb | grep "0483:374b" | awk '{print "/dev/bus/usb/" $2 "/" $4}' | tr -d ':')
sudo chmod 666 $DEVICE

echo "==> Flashing..."
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
    -c "program /workspace/stm32/build/Debug/stm32.elf verify reset exit"
echo "==> Done!"
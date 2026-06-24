#!/bin/bash

set -e

echo "==> Flashing..."
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
    -c "program /workspace/stm32/build/Debug/stm32.elf verify reset exit"
echo "==> Done!"

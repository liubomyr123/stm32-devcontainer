#!/bin/bash

echo "==> Running clang-tidy..."

find /workspace/app -name "*.cpp" -o -name "*.hpp" | xargs clang-tidy \
    --config-file=/workspace/.clang-tidy \
    -p /workspace/stm32/build/Debug \
    --extra-arg=-I/usr/include/newlib/c++/10.3.1 \
    --extra-arg=-I/usr/include/newlib/c++/10.3.1/arm-none-eabi/thumb/v7e-m+dp/hard \
    --extra-arg=-I/usr/lib/gcc/arm-none-eabi/10.3.1/include \
    --extra-arg=-I/usr/lib/arm-none-eabi/include

echo "==> Done!"
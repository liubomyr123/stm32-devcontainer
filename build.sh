#!/bin/bash

set -e

cd /workspace/stm32

echo "==> Cleaning..."
rm -rf build

echo "==> Configuring..."
cmake --preset Debug

echo "==> Building..."
cmake --build --preset Debug

echo "==> Done!"
#!/bin/bash

set -e

echo "==> Setting permissions..."
sudo chmod 666 /dev/ttyACM0

echo "==> Starting monitor..."
picocom -b 115200 /dev/ttyACM0

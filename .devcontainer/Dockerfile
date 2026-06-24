FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

# базові утиліти + ARM toolchain + OpenOCD за один шар
RUN apt-get update && apt-get install -y \
    git \
    curl \
    wget \
    unzip \
    build-essential \
    cmake \
    ninja-build \
    python3 \
    python3-pip \
    libusb-1.0-0 \
    gcc-arm-none-eabi \
    gdb-multiarch \
    openocd \
    udev \
    clangd \
    usbutils \
    && rm -rf /var/lib/apt/lists/*

# ST-Link udev rules
RUN wget https://raw.githubusercontent.com/stlink-org/stlink/master/config/udev/rules.d/49-stlinkv2.rules \
    -O /etc/udev/rules.d/49-stlinkv2.rules

# cleanup
RUN apt-get clean

WORKDIR /workspace
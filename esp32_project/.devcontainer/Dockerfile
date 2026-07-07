FROM espressif/idf:v5.4.1

USER root

RUN apt-get update && apt-get install -y \
    usbutils \
    picocom \
    sudo \
    git \
    clang-format \
    clang-tidy \
    && rm -rf /var/lib/apt/lists/*
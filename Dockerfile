FROM ros:humble

RUN apt-get update && apt-get install -y \
    build-essential \
    curl \
    cmake \
    libgfortran5 \
    libxcb-xkb-dev \
    libcurl4-openssl-dev \
    libbluetooth-dev \
    sharutils \
    python3-pip \
    libsuitesparse-dev \
    bluez \
    ros-humble-rmw-cyclonedds-cpp

ENTRYPOINT [""]

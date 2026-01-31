FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest

# Install build dependencies
RUN apt-get update && apt-get install -y \
    git \
    python3 \
    python3-pip \
    clang \
    libpq-dev \
    libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

# Install AMBuild
RUN pip3 install git+https://github.com/alliedmodders/ambuild

# Set up SDK in /sdk directory
WORKDIR /sdk
RUN git clone https://github.com/alliedmodders/hl2sdk --branch cs2 hl2sdk-cs2 \
    && git clone https://github.com/alliedmodders/metamod-source --recursive

# Set environment variables for SDK paths
ENV HL2SDKCS2=/sdk/hl2sdk-cs2
ENV MMSOURCE112=/sdk/metamod-source

# Set working directory for plugin source
WORKDIR /app

# Default command: configure and build
CMD ["bash", "-c", "mkdir -p build && cd build && CC=clang CXX=clang++ python3 ../configure.py && ambuild"]

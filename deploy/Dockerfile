ARG BASE_IMAGE=joedwards32/cs2:latest

FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest AS build

# gcc-14 (full C++23 std::format, libstdc++ ABI matching Valve's prebuilt SDK
# libs) ships in the sniper_beta apt suite as gcc-14-monolithic; it installs
# gcc-14/g++-14 on PATH. The steamrt base already trusts this repo's signing key.
RUN echo 'deb https://repo.steampowered.com/steamrt3/apt sniper_beta main contrib non-free' \
        > /etc/apt/sources.list.d/steamrt-beta.list \
    && apt-get update && apt-get install -y \
    ca-certificates \
    python3 \
    python3-pip \
    gcc-14-monolithic \
    ninja-build \
    build-essential \
    pkg-config \
    perl \
    autoconf \
    automake \
    libtool \
    && rm -rf /var/lib/apt/lists/*

ARG CMAKE_MIN_VERSION=4.3.4
ARG CONAN_MIN_VERSION=2.29.1
RUN pip3 install "cmake>=${CMAKE_MIN_VERSION}" "conan>=${CONAN_MIN_VERSION}"

WORKDIR /app

FROM ${BASE_IMAGE} AS runtime

USER root

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        tar \
        rsync \
    && rm -rf /var/lib/apt/lists/*

USER steam

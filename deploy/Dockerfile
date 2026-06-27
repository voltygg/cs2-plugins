ARG BASE_IMAGE=joedwards32/cs2:latest

FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest AS build

ARG CMAKE_MIN_VERSION=4.3.4
ARG CONAN_MIN_VERSION=2.29.1

# gcc-14 from sniper_beta provides C++23 support with Valve SDK ABI compatibility.
RUN echo 'deb https://repo.steampowered.com/steamrt3/apt sniper_beta main contrib non-free' \
        > /etc/apt/sources.list.d/steamrt-beta.list \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        autoconf \
        automake \
        ca-certificates \
        gcc-14-monolithic \
        git \
        libtool \
        m4 \
        make \
        ninja-build \
        pkg-config \
        perl \
        python3-pip \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir "cmake>=${CMAKE_MIN_VERSION}" "conan>=${CONAN_MIN_VERSION}"

WORKDIR /app

FROM ${BASE_IMAGE} AS runtime

USER root

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        tar \
    && rm -rf /var/lib/apt/lists/*

USER steam

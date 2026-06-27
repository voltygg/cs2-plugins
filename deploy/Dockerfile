ARG BASE_IMAGE=joedwards32/cs2:latest

FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest AS build

RUN apt-get update && apt-get install -y \
    ca-certificates \
    python3 \
    python3-pip \
    clang \
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

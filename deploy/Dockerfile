ARG BASE_IMAGE=joedwards32/cs2:latest

FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest AS build

RUN apt-get update && apt-get install -y \
    ca-certificates \
    git \
    python3 \
    python3-pip \
    clang \
    cmake \
    libpq-dev \
    libpqxx-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# cpr isn't packaged for the sniper runtime (Debian bullseye); build it from source.
ARG CPR_VERSION=1.11.2
RUN git clone --depth 1 --branch "${CPR_VERSION}" https://github.com/libcpr/cpr.git /tmp/cpr \
    && cmake -S /tmp/cpr -B /tmp/cpr/build \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=ON \
        -DCPR_USE_SYSTEM_CURL=ON \
        -DCPR_BUILD_TESTS=OFF \
    && cmake --build /tmp/cpr/build -j"$(nproc)" \
    && cmake --install /tmp/cpr/build \
    && ldconfig \
    && rm -rf /tmp/cpr

RUN pip3 install git+https://github.com/alliedmodders/ambuild

WORKDIR /app

FROM ${BASE_IMAGE} AS runtime

USER root

# Runtime shared-library closure for the plugin. libcpr.so is copied from the
# build stage (not packaged for this Debian release); libcurl comes via curl.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        tar \
        rsync \
        libpq-dev \
        libpqxx-dev \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /usr/local/lib/libcpr.so* /usr/local/lib/
RUN ldconfig

USER steam

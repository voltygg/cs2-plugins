ARG BASE_IMAGE=joedwards32/cs2:latest

FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest AS build

RUN apt-get update && apt-get install -y \
    ca-certificates \
    git \
    python3 \
    python3-pip \
    clang \
    libpq-dev \
    libpqxx-dev \
    libcpr-dev \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install git+https://github.com/alliedmodders/ambuild

WORKDIR /app

FROM ${BASE_IMAGE} AS runtime

USER root

# Keep the runtime image boring on purpose: the CS2 image owns SteamCMD/server
# lifecycle, while this layer supplies the plugin's shared-library closure and
# small hook-time utilities. CPR may bring libcurl transitively; the project does
# not link libcurl directly.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        tar \
        rsync \
        libpq-dev \
        libpqxx-dev \
        libcpr-dev \
    && rm -rf /var/lib/apt/lists/*

USER steam

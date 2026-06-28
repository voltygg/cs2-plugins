# SteamRT Sniper base image provides a modern toolchain with Valve SDK ABI compatibility
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
  ccache \
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

# CI sets HOME=/github/home; pin CONAN_HOME to the bake's path so CI finds the deps.
ENV CONAN_HOME=/root/.conan2

# Pre-build deps so CI never compiles them; same profiles/build_type/lockfile as
# scripts/build.py so package IDs match (else cache misses, deps rebuild).
COPY conanfile.py conan.lock /app/
COPY conan/profiles /app/conan/profiles
RUN conan install /app --build=missing --lockfile /app/conan.lock \
  --profile:host /app/conan/profiles/linux-steamrt.txt \
  --profile:build /app/conan/profiles/linux-steamrt.txt \
  -s build_type=Release --output-folder /tmp/conan-bake \
  && conan cache clean "*" --source --build --temp \
  && rm -rf /tmp/conan-bake

WORKDIR /app

# CS2 Server runtime
FROM joedwards32/cs2:latest AS runtime

USER root

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  ca-certificates \
  curl \
  tar \
  && rm -rf /var/lib/apt/lists/*

# Vars the base entry.sh reads under nounset but sets only conditionally. Empty = off.
ENV CS2_PW="" \
  CS2_HOST_WORKSHOP_COLLECTION="" \
  CS2_HOST_WORKSHOP_MAP="" \
  CS2_BOT_QUOTA="" \
  CS2_BOT_QUOTA_MODE="" \
  CS2_BOT_DIFFICULTY="" \
  CS2_HOST_WORKSHOP_COLLECTION_ARGS="" \
  CS2_HOST_WORKSHOP_MAP_ARGS="" \
  CS2_MP_MATCH_END_CHANGELEVEL="" \
  CS2_PW_ARGS="" \
  SV_SETSTEAMACCOUNT_ARGS=""

USER steam

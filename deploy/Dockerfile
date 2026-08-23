# SteamRT Sniper base image provides a modern toolchain with Valve SDK ABI compatibility
FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest AS build

# The cs2-kit Python distribution: the `cs2kit` command plus the pinned CMake, Conan,
# Ninja and clang-format. Move this to a v* tag when reproducibility matters more than
# tracking the kit.
ARG CS2KIT_REF=main

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
  openssh-client \
  pkg-config \
  perl \
  python3-pip \
  && rm -rf /var/lib/apt/lists/*

# pyyaml is the deploy inventory reader's one dependency; jobs in this image call
# `deploy.tools.cli package --all`, and installing it per run is pure latency.
RUN pip3 install --no-cache-dir pyyaml \
  "cs2-kit @ git+https://github.com/voltygg/cs2-kit.git@${CS2KIT_REF}"

# CI sets HOME=/github/home; pin CONAN_HOME to the bake's path so CI finds the deps.
ENV CONAN_HOME=/root/.conan2

# Profiles and the public remote, canonical in cs2-kit - the same one command a
# developer runs. No credentials: the Cloudsmith repo is public.
RUN conan config install https://github.com/voltygg/cs2-kit.git -sf conan

# Pre-build deps so CI never compiles them; same profile/build_type/lockfile as
# `cs2kit build`, so package IDs match (else every dep is a cache miss and rebuilds).
COPY conanfile.py conan.lock /app/
RUN conan install /app --build=missing \
  '--build=!hl2sdk-cs2/*' '--build=!metamod-source/*' \
  --lockfile /app/conan.lock \
  --profile:host linux-steamrt.txt --profile:build linux-steamrt.txt \
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

# Base entry.sh runs under `set -u` and reads these without defaulting them, so
# any unset crashes the container ("unbound variable"). Empty = feature off.
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
  CS2_ADDITIONAL_ARGS="" \
  SV_SETSTEAMACCOUNT_ARGS=""

USER steam

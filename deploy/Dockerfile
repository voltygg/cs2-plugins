# SteamRT sniper: Valve SDK ABI compatibility.
FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest AS build

# Move to a v* tag when reproducibility matters more than tracking the kit.
ARG VOLTMOD_REF=main

# gcc-14 (C++23) lives in sniper_beta, not the base image.
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
  && rm -rf /var/lib/apt/lists/*

# voltmod's venv is `python3` and carries the pinned CMake/Conan/Ninja/clang-format, so
# no stage below names a tool version. uv fetches 3.14; sniper only ships 3.9. pyyaml is
# for `deploy.tools.cli package --all`, which jobs in this image run.
# Both install dirs stay off $HOME: CI rebinds it to /github/home.
COPY --from=ghcr.io/astral-sh/uv:latest /uv /usr/local/bin/uv
ENV PATH=/opt/voltmod/bin:$PATH UV_PYTHON_INSTALL_DIR=/opt/uv-python CONAN_HOME=/root/.conan2
RUN uv venv --python 3.14 /opt/voltmod \
  && uv pip install --python /opt/voltmod/bin/python --no-cache pyyaml \
  "voltmod @ git+https://github.com/voltygg/voltmod.git@${VOLTMOD_REF}"

# Profiles and the public remote, canonical in voltmod. No credentials: the repo is public.
RUN conan config install https://github.com/voltygg/voltmod.git -sf conan

# Pre-build deps so CI never compiles them; same profile/build_type/lockfile as
# `voltmod build`, so package IDs match (else every dep is a cache miss and rebuilds).
COPY conanfile.py conan.lock version.txt /app/
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

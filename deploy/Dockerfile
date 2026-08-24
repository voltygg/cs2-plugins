FROM registry.gitlab.steamos.cloud/steamrt/sniper/sdk:latest AS build

ARG VOLTMOD_REF=main

RUN echo 'deb https://repo.steampowered.com/steamrt3/apt sniper_beta main contrib non-free' \
  > /etc/apt/sources.list.d/steamrt-beta.list \
  && apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  ca-certificates \
  ccache \
  gcc-14-monolithic \
  git \
  ninja-build \
  && rm -rf /var/lib/apt/lists/*

COPY --from=ghcr.io/astral-sh/uv:latest /uv /usr/local/bin/uv
ENV PATH=/opt/voltmod/bin:$PATH \
  VIRTUAL_ENV=/opt/voltmod \
  UV_PYTHON_INSTALL_DIR=/opt/uv-python \
  CONAN_HOME=/root/.conan2

RUN uv venv --python 3.14 /opt/voltmod \
  && uv pip install --python /opt/voltmod/bin/python --no-cache \
  "voltmod @ git+https://github.com/voltygg/voltmod.git@${VOLTMOD_REF}"

RUN conan config install https://github.com/voltygg/voltmod.git -sf conan

COPY conanfile.py conan.lock version.txt /app/
RUN conan install /app --build=missing \
  '--build=!hl2sdk-cs2/*' '--build=!metamod-source/*' \
  --lockfile /app/conan.lock \
  --profile:host linux-steamrt.txt --profile:build linux-steamrt.txt \
  -s build_type=Release --output-folder /tmp/conan-bake \
  && conan cache clean "*" --source --build --temp \
  && rm -rf /tmp/conan-bake

WORKDIR /app

FROM joedwards32/cs2:latest AS runtime

USER root

RUN apt-get update \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
  ca-certificates \
  curl \
  tar \
  && rm -rf /var/lib/apt/lists/*

# Defaults required by the base image's `set -u` entrypoint.
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

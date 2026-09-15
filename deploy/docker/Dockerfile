# Runtime image for Docker hosts: joedwards32/cs2 plus the tools pre.sh needs.
FROM joedwards32/cs2:latest

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

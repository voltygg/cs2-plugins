ARG BASE_IMAGE=joedwards32/cs2:latest
FROM ${BASE_IMAGE}

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

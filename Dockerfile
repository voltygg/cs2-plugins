FROM registry.gitlab.steamos.cloud/steamrt/sniper/platform:latest

# Install build dependencies
RUN apt-get update && apt-get install -y \
  git \
  python3 \
  python3-pip \
  clang \
  libpq-dev \
  libpqxx-dev \
  && rm -rf /var/lib/apt/lists/*

# Install AMBuild
RUN pip3 install git+https://github.com/alliedmodders/ambuild

# Set working directory for plugin source
WORKDIR /app

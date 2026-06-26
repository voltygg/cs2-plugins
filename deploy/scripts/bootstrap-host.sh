#!/usr/bin/env bash
#
# Minimal one-time host prep for the Docker deployment path. Run as root on a
# fresh Ubuntu box, then deploy with deploy/scripts/deploy.sh.

set -euo pipefail

DeployUser="${DEPLOY_USER:-steam}"
DeployRoot="${DEPLOY_ROOT:-/opt/cs2}"
SshPort="${SSH_PORT:-22}"
PortRange="${CS2_PORT_RANGE:-27015:27035}"

log() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
die() { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

[[ "${EUID:-$(id -u)}" -eq 0 ]] || die "must run as root"

log "Installing Docker, Compose plugin, firewall, and rsync"
apt-get update -y
DEBIAN_FRONTEND=noninteractive apt-get install -y \
    ca-certificates curl docker.io docker-compose-plugin rsync ufw

if id -u "$DeployUser" >/dev/null 2>&1; then
    log "user '$DeployUser' already exists"
else
    useradd -m -s /bin/bash "$DeployUser"
    log "created user '$DeployUser'"
fi

usermod -aG docker "$DeployUser"
install -d -o "$DeployUser" -g "$DeployUser" "$DeployRoot"

log "Configuring ufw (SSH $SshPort, CS2 $PortRange)"
ufw default deny incoming
ufw default allow outgoing
ufw allow "${SshPort}/tcp"
ufw allow "${PortRange}/udp"
ufw allow "${PortRange}/tcp"
ufw --force enable

systemctl enable --now docker
log "Docker host ready. Log out/in for '$DeployUser' docker group membership to apply."

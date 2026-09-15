#!/usr/bin/env bash
# Prepare an Ubuntu 24.04+ Docker host. Run as root, then use
# `uv run poe deploy-server`. Pass `--skip-docker` for a managed Docker install.

set -euo pipefail

DeployUser="${DEPLOY_USER:-steam}"
Cs2Root="${CS2_ROOT:-/home/$DeployUser/cs2}"
DeployRoot="${DEPLOY_ROOT:-$Cs2Root/deploy}"
SshPort="${SSH_PORT:-22}"
PortRange="${CS2_PORT_RANGE:-27015:27035}"
SkipDocker=0

log() { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
die() { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

while [[ $# -gt 0 ]]; do
    case "$1" in
        --skip-docker) SkipDocker=1; shift ;;
        -h|--help) sed -n '2,7p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *) die "unknown argument: $1" ;;
    esac
done

[[ "${EUID:-$(id -u)}" -eq 0 ]] || die "must run as root"

Packages=(rsync ufw)
if [[ "$SkipDocker" -eq 0 ]] && command -v docker >/dev/null 2>&1; then
    log "docker already present; skipping Docker install"
    SkipDocker=1
fi
[[ "$SkipDocker" -eq 1 ]] || Packages+=(docker.io docker-compose-v2)

log "Installing: ${Packages[*]}"
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y "${Packages[@]}"

command -v docker >/dev/null 2>&1 || die "docker not found (install it or drop --skip-docker)"
docker compose version >/dev/null 2>&1 || die "docker compose v2 plugin missing (install docker-compose-v2)"

if id -u "$DeployUser" >/dev/null 2>&1; then
    log "user '$DeployUser' already exists"
else
    useradd -m -s /bin/bash "$DeployUser"
    log "created user '$DeployUser'"
fi

usermod -aG docker "$DeployUser"
install -d -o "$DeployUser" -g "$DeployUser" "$DeployRoot" "$Cs2Root/server"

log "Configuring ufw (SSH $SshPort, CS2 $PortRange/udp)"
ufw default deny incoming
ufw default allow outgoing
ufw allow "${SshPort}/tcp"
# CS2 game traffic is UDP; RCON (TCP) is intentionally not exposed publicly.
ufw allow "${PortRange}/udp"
ufw --force enable

log "Docker host ready. Log out/in for '$DeployUser' docker group membership to apply."

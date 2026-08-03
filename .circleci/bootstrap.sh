#!/usr/bin/env bash
# Creates the CircleCI contexts this project's config.yml expects and fills them
# from local files. Safe to re-run: storing a secret overwrites the old value, so
# this doubles as the rotation script.
#
# Prereqs:
#   winget install CircleCI-Public.CircleCI-CLI
#   circleci setup                  # paste a personal API token
#
# Usage (run from the repo root, in Git Bash - not PowerShell):
#   ./.circleci/bootstrap.sh
#   CIRCLE_ORG_ID=<uuid> ./.circleci/bootstrap.sh   # GitHub App orgs
#
# Per-server env files come from deploy/secrets/servers/<id>/.env for every
# active server in deploy/inventory.yml, so adding a box needs no edit here.
set -euo pipefail

ORG_NAME="${CIRCLE_ORG_NAME:-voltygg}"
# Matches SSH_KEY_FILE in deploy/secrets/servers/box-a/.env
SSH_KEY_PATH="${SSH_KEY_PATH:-$HOME/.ssh/mehnatsevar_deploy}"

command -v circleci >/dev/null || {
  echo "ERROR: circleci CLI not found. winget install CircleCI-Public.CircleCI-CLI" >&2
  exit 1
}

if [ -n "${CIRCLE_ORG_ID:-}" ]; then
  ORG=(--org-id "$CIRCLE_ORG_ID")
else
  ORG=(github "$ORG_NAME")
fi

ensure_context() {
  circleci context create "${ORG[@]}" "$1" 2>/dev/null || echo "  context '$1' already exists"
}

# Value arrives on stdin so it never lands in the process table or shell history.
store() {
  echo "  $2"
  circleci context store-secret "${ORG[@]}" "$1" "$2" >/dev/null
}

# Reads into the named variable unless it is already set in the environment.
ask() {
  local var="$1" prompt="$2" hidden="${3:-}"
  [ -n "${!var:-}" ] && return 0
  if [ -n "$hidden" ]; then
    read -rsp "$prompt: " "$var" && echo
  else
    read -rp "$prompt: " "$var"
  fi
  [ -n "${!var:-}" ] || { echo "ERROR: $var is required" >&2; exit 1; }
}

# CRLF would survive base64 and corrupt the decoded file on the Linux box.
b64_file() {
  [ -f "$1" ] || { echo "ERROR: $1 not found" >&2; exit 1; }
  tr -d '\r' < "$1" | base64 -w0
}

ask GHCR_USERNAME "GitHub username (GHCR push identity)"
ask GHCR_TOKEN    "GHCR PAT with read+write:packages" hidden

echo "==> context: ghcr"
ensure_context ghcr
printf '%s' "$GHCR_USERNAME" | store ghcr GHCR_USERNAME
printf '%s' "$GHCR_TOKEN"    | store ghcr GHCR_TOKEN

echo "==> context: cs2-deploy"
ensure_context cs2-deploy
b64_file "$SSH_KEY_PATH" | store cs2-deploy SSH_KEY_B64

for id in $(python3 -m deploy.tools.cli matrix --format plain); do
  var="SERVER_ENV_$(echo "$id" | tr '[:lower:]-' '[:upper:]_')_B64"
  b64_file "deploy/secrets/servers/$id/.env" | store cs2-deploy "$var"
done

cat <<'EOF'

Contexts are populated. Remaining dashboard steps:
  1. Project Settings -> Advanced -> Auto-cancel redundant workflows: OFF
     (a second prod push must not kill a deploy mid rsync)
  2. Set the project up against .circleci/config.yml if you have not already
EOF

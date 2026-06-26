# Shared helpers + paths for the provisioning scripts. Source, don't execute.
# Override any of the STEAM_*/CS2_* paths via the environment before running.

STEAM_USER="${STEAM_USER:-steam}"
STEAM_HOME="${STEAM_HOME:-/home/steam}"
STEAMCMD_DIR="${STEAMCMD_DIR:-$STEAM_HOME/steamcmd}"
CS2_DIR="${CS2_DIR:-$STEAM_HOME/cs2}"
BACKUP_DIR="${BACKUP_DIR:-$STEAM_HOME/backups}"

log()  { printf '\033[1;34m==>\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33mWARN:\033[0m %s\n' "$*" >&2; }
die()  { printf '\033[1;31mERROR:\033[0m %s\n' "$*" >&2; exit 1; }

require_root() {
    [[ "${EUID:-$(id -u)}" -eq 0 ]] || die "must run as root (use sudo)"
}

# Insert the Metamod search path into gameinfo.gi, idempotently.
patch_gameinfo() {
    local f="$1"
    [[ -f "$f" ]] || die "gameinfo.gi not found: $f"
    if grep -q 'csgo/addons/metamod' "$f"; then
        log "gameinfo.gi already references addons/metamod"
        return 0
    fi
    awk '
        !done && $0 ~ /[[:space:]]Game[[:space:]]+csgo[[:space:]]*$/ {
            match($0, /^[[:space:]]*/); indent = substr($0, 1, RLENGTH)
            printf "%sGame\tcsgo/addons/metamod\n", indent
            done = 1
        }
        { print }
    ' "$f" > "$f.tmp" && mv "$f.tmp" "$f"
    grep -q 'csgo/addons/metamod' "$f" || die "failed to patch $f"
    log "patched gameinfo.gi (added Game csgo/addons/metamod)"
}

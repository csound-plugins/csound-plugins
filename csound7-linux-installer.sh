#!/usr/bin/env bash
set -euo pipefail

# ═══════════════════════════════════════════════════════════════════
# Csound 7 Portable Linux - One-line installer
#
# Usage:
#   curl -fsSL https://raw.githubusercontent.com/csound-plugins/csound-plugins/master/install-csound7-linux.sh | bash
#
# This script downloads the release asset named in CSOUND7_ASSET
# (default: csound7-linux-portable-with-plugins.zip), extracts it,
# and runs the bundled install.sh.
# ═══════════════════════════════════════════════════════════════════

REPO="csound-plugins/csound-plugins"
TAG="${CSOUND7_TAG:-latest}"
ASSET="${CSOUND7_ASSET:-csound7-linux-full.zip}"
DOWNLOAD_URL="https://github.com/${REPO}/releases/download/${TAG}/${ASSET}"

# ─── Helpers ──────────────────────────────────────────────────────
error() {
    printf 'Error: %s\n' "$*" >&2
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        error "'$1' is required but not installed."
        error "Please install it and try again (e.g. sudo apt install $1)."
        exit 1
    fi
}

# ─── Check dependencies ───────────────────────────────────────────
require_command curl
require_command unzip
require_command mktemp

# ─── Prepare temporary directory ──────────────────────────────────
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

ZIP_FILE="${TMP_DIR}/${ASSET}"
EXTRACT_DIR="${TMP_DIR}/extracted"
mkdir -p "$EXTRACT_DIR"

# ─── Download ─────────────────────────────────────────────────────
echo "Downloading ${ASSET} from GitHub..."
if ! curl -fsSL -o "$ZIP_FILE" "$DOWNLOAD_URL"; then
    error "Failed to download ${ASSET}"
    error "URL: ${DOWNLOAD_URL}"
    exit 1
fi
echo "Download complete: ${ZIP_FILE}"

# ─── Extract ──────────────────────────────────────────────────────
echo "Extracting ${ASSET}..."
if ! unzip -q "$ZIP_FILE" -d "$EXTRACT_DIR"; then
    error "Failed to extract ${ASSET}"
    exit 1
fi

# ─── Locate bundled installer ─────────────────────────────────────
INSTALLER=""
while IFS= read -r -d '' candidate; do
    INSTALLER="$candidate"
    break
done < <(find "$EXTRACT_DIR" -name install.sh -type f -print0)

if [[ -z "$INSTALLER" ]]; then
    error "install.sh was not found inside ${ASSET}"
    exit 1
fi

chmod +x "$INSTALLER"

# ─── Run installer ────────────────────────────────────────────────
echo "Running bundled installer: ${INSTALLER}"
if [[ -t 0 ]]; then
    # stdin is a terminal: run normally
    "$INSTALLER" "$@"
else
    # Executed via curl | bash: give the interactive installer a TTY
    # so its prompts (read/sudo) work correctly.
    if [[ -e /dev/tty ]]; then
        "$INSTALLER" "$@" < /dev/tty
    else
        error "No terminal available (/dev/tty)."
        error "This installer is interactive; please run it from a terminal."
        exit 1
    fi
fi

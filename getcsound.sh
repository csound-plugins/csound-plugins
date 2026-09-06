#!/usr/bin/env bash
set -euo pipefail

usage() {
    cat <<'EOF'
Usage: install.sh [OPTIONS] [-- BUNDLED-INSTALLER-OPTIONS]

Options:
  --help       Show this help without downloading the installer.
  --help-all   Download the installer and show this help plus the bundled installer help.
  --verbose    Show the checksum URL and expected and calculated SHA-256 checksums.

Arguments after -- are passed unchanged to the bundled installer. Use --help-all
to see the bundled installer's supported options and parameters.
EOF
}

VERBOSE=0
SHOW_HELP=0
HELP_ALL=0
INSTALLER_ARGS=()
while (($#)); do
    case "$1" in
        --help)
            SHOW_HELP=1
            ;;
        --help-all)
            HELP_ALL=1
            ;;
        --verbose)
            VERBOSE=1
            ;;
        --)
            shift
            INSTALLER_ARGS+=("$@")
            break
            ;;
        *)
            INSTALLER_ARGS+=("$1")
            ;;
    esac
    shift
done

if ((SHOW_HELP)); then
    usage
    exit 0
fi

# Detect the operating system.
OS=$(uname -s)
case "$OS" in
    Linux)
        ;;
    Darwin)
        echo "macOS is not supported at the moment. Only Linux is supported."
        exit 1
        ;;
    *)
        echo "Unsupported operating system: $OS. Only Linux is supported."
        exit 1
        ;;
esac

# ═══════════════════════════════════════════════════════════════════
# Csound 7 Portable Linux - One-line installer
#
# SAFER USAGE (recommended):
#   curl -fsSL -o install-csound7-linux.sh \
#       https://csound-plugins.github.io/installer/install.sh
#   # Read the script, then run it:
#   bash ./install-csound7-linux.sh
#
# One-line usage (convenient, but inspects nothing before execution):
#   curl -fsSL https://csound-plugins.github.io/installer/install.sh | bash
#
# This script downloads the release asset named in CSOUND7_ASSET
# (default: csound7-linux-full.zip), verifies its SHA-256 checksum,
# extracts it, and runs the bundled install.sh.
#
# Trust model:
#   - The release archive is downloaded over HTTPS from GitHub.
#   - A SHA-256 checksum file (${ASSET}.sha256) is downloaded from the
#     same release and verified before extraction.
#   - Checksums guard against corruption and trivial modification, but
#     they are hosted in the same place as the archive. For stronger
#     trust, sign the checksum file with GPG and verify it here.
# ═══════════════════════════════════════════════════════════════════

# Detect the CPU architecture.
ARCH=$(uname -m)
case "$ARCH" in
    x86_64|amd64)
        ARCH_SUFFIX="x86_64"
        ;;
    aarch64|arm64)
        ARCH_SUFFIX="aarch64"
        ;;
    *)
        echo "Unsupported architecture: $ARCH. Supported architectures: x86_64, aarch64."
        exit 1
        ;;
esac

REPO="csound-plugins/csound-plugins"
TAG="${CSOUND7_TAG:-latest}"
ASSET="${CSOUND7_ASSET:-csound7-linux-${ARCH_SUFFIX}.zip}"
CHECKSUM_ASSET="${ASSET}.sha256"
DOWNLOAD_URL="https://github.com/${REPO}/releases/download/${TAG}/${ASSET}"
CHECKSUM_URL="https://github.com/${REPO}/releases/download/${TAG}/${CHECKSUM_ASSET}"

# ─── Helpers ──────────────────────────────────────────────────────
error() {
    printf 'Error: %s\n' "$*" >&2
}

verbose() {
    if ((VERBOSE)); then
        printf '%s\n' "$*"
    fi
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

# sha256sum is preferred; macOS's shasum is not used here because Linux is required,
# but keep a fallback so the script is portable in case that restriction changes.
if command -v sha256sum >/dev/null 2>&1; then
    SHA256_CMD="sha256sum"
elif command -v shasum >/dev/null 2>&1; then
    SHA256_CMD="shasum -a 256"
else
    error "A SHA-256 checksum tool is required (sha256sum or shasum)."
    exit 1
fi

# ─── Prepare temporary directory ──────────────────────────────────
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

ZIP_FILE="${TMP_DIR}/${ASSET}"
EXTRACT_DIR="${TMP_DIR}/extracted"
mkdir -p "$EXTRACT_DIR"

# ─── Download ─────────────────────────────────────────────────────
echo "Downloading ${ASSET}..."
echo "URL: ${DOWNLOAD_URL}"
if ! curl -fsSL -o "$ZIP_FILE" "$DOWNLOAD_URL"; then
    error "Failed to download ${ASSET}"
    error "URL: ${DOWNLOAD_URL}"
    exit 1
fi
echo "Download complete: ${ZIP_FILE}"

# ─── Verify checksum ──────────────────────────────────────────────
echo "Downloading checksum file: ${CHECKSUM_ASSET}..."
verbose "Checksum URL: ${CHECKSUM_URL}"
if curl -fsSL -o "${ZIP_FILE}.sha256" "$CHECKSUM_URL"; then
    echo "Verifying SHA-256 checksum..."
    EXPECTED_HASH=$(awk 'NR==1 {print $1}' "${ZIP_FILE}.sha256")
    ACTUAL_HASH=$($SHA256_CMD "$ZIP_FILE" | awk '{print $1}')
    verbose "Expected SHA-256: ${EXPECTED_HASH:-<missing>}"
    verbose "Actual SHA-256:   ${ACTUAL_HASH:-<missing>}"
    if [[ -z "$EXPECTED_HASH" ]] || [[ "$EXPECTED_HASH" != "$ACTUAL_HASH" ]]; then
        error "SHA-256 checksum verification failed for ${ASSET}"
        error "Expected: ${EXPECTED_HASH:-<missing>}"
        error "Actual:   ${ACTUAL_HASH:-<missing>}"
        error "The downloaded file may be corrupted or have been modified."
        error "Checksum URL: ${CHECKSUM_URL}"
        exit 1
    fi
    echo "Checksum verified."
else
    error "Failed to download checksum file: ${CHECKSUM_ASSET}"
    error "URL: ${CHECKSUM_URL}"
    error "Refusing to install an unverified archive."
    exit 1
fi

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

if ((HELP_ALL)); then
    usage
    printf '\nBundled installer help:\n'
    "$INSTALLER" --help
    exit 0
fi

# ─── Run installer ────────────────────────────────────────────────
echo "Running bundled installer: ${INSTALLER}"
if [[ -t 0 ]]; then
    # stdin is a terminal: run normally
    "$INSTALLER" "${INSTALLER_ARGS[@]}"
else
    # Executed via curl | bash: give the interactive installer a TTY
    # so its prompts (read/sudo) work correctly.
    if [[ -e /dev/tty ]]; then
        "$INSTALLER" "${INSTALLER_ARGS[@]}" < /dev/tty
    else
        error "No terminal available (/dev/tty)."
        error "This installer is interactive; please run it from a terminal."
        exit 1
    fi
fi

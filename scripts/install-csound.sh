#!/usr/bin/env bash
set -euo pipefail

# ─────────────────────────────────────────────────────────────
# Csound 7 Portable Linux Installer
# ─────────────────────────────────────────────────────────────

RELEASE_URL="https://github.com/csound-plugins/csound-plugins/releases/download/latest/csound7-linux-portable-with-plugins.zip"
ZIP_NAME="csound7-linux-portable-with-plugins.zip"

INSTALL_DIR="$HOME/.local/csound"
PLUGIN_DIR="$HOME/.local/lib/csound/7.0/plugins64"
LIB_NAME="libcsound64.so.7.0"
LIB_SYMLINK="libcsound64.so"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

info()    { printf "${BLUE}ℹ %s${NC}\n" "$*"; }
ok()      { printf "${GREEN}✔ %s${NC}\n" "$*"; }
warn()    { printf "${YELLOW}⚠ %s${NC}\n" "$*"; }
error()   { printf "${RED}✖ %s${NC}\n" "$*" >&2; }

ask_yes_no() {
    local prompt="$1" response
    while true; do
        read -rp "$prompt [y/N]: " response
        case "$response" in
            [Yy]* ) return 0 ;;
            [Nn]* | "" ) return 1 ;;
            * ) echo "Please answer yes or no." ;;
        esac
    done
}

# ─── Detect shell and config file ───────────────────────────
detect_shell() {
    if [ -n "${SHELL:-}" ]; then
        basename "$SHELL"
    else
        echo "bash"
    fi
}

detect_shell_rc() {
    local shell_name="$1"
    case "$shell_name" in
        bash)
            if [ -f "$HOME/.bashrc" ]; then echo "$HOME/.bashrc"
            elif [ -f "$HOME/.bash_profile" ]; then echo "$HOME/.bash_profile"
            else echo "$HOME/.bashrc"; fi
            ;;
        zsh)
            echo "$HOME/.zshrc"
            ;;
        fish)
            echo "$HOME/.config/fish/config.fish"
            ;;
        *)
            if [ -f "$HOME/.bashrc" ]; then echo "$HOME/.bashrc"
            elif [ -f "$HOME/.zshrc" ]; then echo "$HOME/.zshrc"
            else echo "$HOME/.bashrc"; fi
            ;;
    esac
}

# ─── Shell-specific config generators ───────────────────────
# Returns the line(s) to add to the shell config for PATH
path_config_for_shell() {
    local shell_name="$1"
    case "$shell_name" in
        fish)
            echo "fish_add_path $INSTALL_DIR"
            ;;
        *)
            echo "export PATH=\"$INSTALL_DIR:\$PATH\""
            ;;
    esac
}

# Returns the line(s) to add to the shell config for LIBCSOUNDPATH
libpath_config_for_shell() {
    local shell_name="$1" libpath="$2"
    case "$shell_name" in
        fish)
            echo "set -gx LIBCSOUNDPATH \"$libpath\""
            ;;
        *)
            echo "export LIBCSOUNDPATH=\"$libpath\""
            ;;
    esac
}

# Remove existing LIBCSOUNDPATH lines from shell config
remove_libpath_from_config() {
    local rc_file="$1" shell_name="$2"
    if [ ! -f "$rc_file" ]; then return; fi

    case "$shell_name" in
        fish)
            # Fish: remove lines containing LIBCSOUNDPATH=
            sed -i.bak '/LIBCSOUNDPATH=/d' "$rc_file" && rm -f "$rc_file.bak"
            ;;
        *)
            sed -i.bak '/LIBCSOUNDPATH=/d' "$rc_file" && rm -f "$rc_file.bak"
            ;;
    esac
}

# Check if PATH already contains INSTALL_DIR in shell config
path_already_in_config() {
    local rc_file="$1" shell_name="$2"
    if [ ! -f "$rc_file" ]; then return 1; fi

    case "$shell_name" in
        fish)
            grep -q "fish_add_path.*$INSTALL_DIR" "$rc_file" 2>/dev/null
            ;;
        *)
            grep -q "$INSTALL_DIR" "$rc_file" 2>/dev/null
            ;;
    esac
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# ─── Pre-flight checks ──────────────────────────────────────
info "Checking prerequisites..."

if ! command_exists curl; then
    error "curl is required but not installed."
    exit 1
fi

if ! command_exists unzip; then
    error "unzip is required but not installed."
    exit 1
fi

# ─── Detect shell early ─────────────────────────────────────
SHELL_NAME=$(detect_shell)
SHELL_RC=$(detect_shell_rc "$SHELL_NAME")
info "Detected shell: $SHELL_NAME"
info "Shell config: $SHELL_RC"

# ─── Check existing directories ─────────────────────────────
info "Checking for existing installations..."

CONFLICT=false
if [ -d "$INSTALL_DIR" ]; then
    warn "Directory already exists: $INSTALL_DIR"
    CONFLICT=true
fi

if [ -d "$PLUGIN_DIR" ]; then
    warn "Directory already exists: $PLUGIN_DIR"
    CONFLICT=true
fi

if [ "$CONFLICT" = true ]; then
    if ! ask_yes_no "One or more target directories already exist. Overwrite/install anyway?"; then
        info "Installation cancelled."
        exit 0
    fi
fi

# ─── Check LIBCSOUNDPATH ────────────────────────────────────
info "Checking environment variable LIBCSOUNDPATH..."

if [ -n "${LIBCSOUNDPATH:-}" ]; then
    warn "LIBCSOUNDPATH is already set to: $LIBCSOUNDPATH"
    if ! ask_yes_no "LIBCSOUNDPATH is already defined. Overwrite it?"; then
        info "Keeping existing LIBCSOUNDPATH."
        UPDATE_LIBPATH=false
    else
        UPDATE_LIBPATH=true
    fi
else
    UPDATE_LIBPATH=true
fi

# ─── Download ───────────────────────────────────────────────
TMP_DIR=$(mktemp -d)
trap 'rm -rf "$TMP_DIR"' EXIT

info "Downloading Csound portable build..."
if ! curl -fsSL -o "$TMP_DIR/$ZIP_NAME" "$RELEASE_URL"; then
    error "Failed to download $ZIP_NAME"
    exit 1
fi
ok "Download complete."

# ─── Extract ────────────────────────────────────────────────
info "Extracting archive..."
unzip -q "$TMP_DIR/$ZIP_NAME" -d "$TMP_DIR/extracted"

EXTRACTED_ROOT="$TMP_DIR/extracted"
if [ -d "$EXTRACTED_ROOT/csound" ]; then
    EXTRACTED_ROOT="$EXTRACTED_ROOT/csound"
fi

if [ ! -f "$EXTRACTED_ROOT/csound" ]; then
    CSOUND_BIN=$(find "$TMP_DIR/extracted" -name "csound" -type f | head -n1)
    if [ -z "$CSOUND_BIN" ]; then
        error "Could not find 'csound' executable in archive."
        exit 1
    fi
    EXTRACTED_ROOT=$(dirname "$CSOUND_BIN")
fi

if [ ! -f "$EXTRACTED_ROOT/$LIB_NAME" ]; then
    error "Could not find '$LIB_NAME' in archive."
    exit 1
fi

ok "Archive validated."

# ─── Ensure shell config directory exists ───────────────────
if [ "$SHELL_NAME" = "fish" ]; then
    mkdir -p "$(dirname "$SHELL_RC")"
fi

# ─── Install binaries and library ───────────────────────────
info "Installing to $INSTALL_DIR..."

rm -rf "$INSTALL_DIR"
mkdir -p "$INSTALL_DIR"

cp "$EXTRACTED_ROOT/csound" "$INSTALL_DIR/csound"
chmod +x "$INSTALL_DIR/csound"
cp "$EXTRACTED_ROOT/$LIB_NAME" "$INSTALL_DIR/$LIB_NAME"
ln -sf "$INSTALL_DIR/$LIB_NAME" "$INSTALL_DIR/$LIB_SYMLINK"

ok "Installed: csound, $LIB_NAME, $LIB_SYMLINK"

# ─── Install plugins ────────────────────────────────────────
info "Installing plugins to $PLUGIN_DIR..."

rm -rf "$PLUGIN_DIR"
mkdir -p "$PLUGIN_DIR"

if [ -d "$EXTRACTED_ROOT/plugins" ]; then
    cp -a "$EXTRACTED_ROOT/plugins/." "$PLUGIN_DIR/"
    ok "Plugins installed."
else
    warn "No 'plugins' folder found in archive."
fi

# ─── Update PATH ────────────────────────────────────────────
PATH_LINE=$(path_config_for_shell "$SHELL_NAME")

if path_already_in_config "$SHELL_RC" "$SHELL_NAME"; then
    ok "PATH already contains $INSTALL_DIR in $SHELL_RC"
else
    info "Adding $INSTALL_DIR to PATH in $SHELL_RC..."
    echo "" >> "$SHELL_RC"
    echo "# Added by Csound 7 installer" >> "$SHELL_RC"
    echo "$PATH_LINE" >> "$SHELL_RC"
    ok "PATH updated in $SHELL_RC"
fi

# ─── Update LIBCSOUNDPATH ───────────────────────────────────
if [ "$UPDATE_LIBPATH" = true ]; then
    LIBPATH_LINE=$(libpath_config_for_shell "$SHELL_NAME" "$INSTALL_DIR/$LIB_NAME")

    remove_libpath_from_config "$SHELL_RC" "$SHELL_NAME"

    echo "$LIBPATH_LINE" >> "$SHELL_RC"
    ok "LIBCSOUNDPATH set to: $INSTALL_DIR/$LIB_NAME"
else
    info "Skipped updating LIBCSOUNDPATH."
fi

# ─── Verify ─────────────────────────────────────────────────
info "Verifying installation..."

if [ -x "$INSTALL_DIR/csound" ]; then
    CSOUND_VERSION=$("$INSTALL_DIR/csound" --version 2>&1 | head -n1 || true)
    ok "Csound executable is working."
    info "Version: $CSOUND_VERSION"
else
    warn "Csound executable exists but may not be executable."
fi

# ─── Summary ────────────────────────────────────────────────
echo ""
echo "═══════════════════════════════════════════════════════"
echo "  Csound 7 Portable Installation Complete"
echo "═══════════════════════════════════════════════════════"
echo ""
echo "  Shell detected:     $SHELL_NAME"
echo "  Shell config:       $SHELL_RC"
echo "  Install directory:  $INSTALL_DIR"
echo "  Plugin directory:   $PLUGIN_DIR"
echo ""
echo "  To use Csound immediately in this terminal, run:"
if [ "$SHELL_NAME" = "fish" ]; then
    echo "    source $SHELL_RC"
else
    echo "    source $SHELL_RC"
fi
echo ""
echo "  Or open a new terminal session."
echo ""
echo "  Test with:"
echo "    csound --version"
echo "═══════════════════════════════════════════════════════"

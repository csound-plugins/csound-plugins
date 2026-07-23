#!/usr/bin/env bash
set -euo pipefail

# ═══════════════════════════════════════════════════════════════
# Csound 7 Portable Linux Installer
# Shipped inside: csound7-linux-portable-with-plugins.zip
# ═══════════════════════════════════════════════════════════════

# ─── Colors ───────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

info()    { printf "${BLUE}ℹ %s${NC}\n" "$*"; }
ok()      { printf "${GREEN}✔ %s${NC}\n" "$*"; }
warn()    { printf "${YELLOW}⚠ %s${NC}\n" "$*"; }
error()   { printf "${RED}✖ %s${NC}\n" "$*" >&2; }
header()  { printf "${CYAN}%s${NC}\n" "$*"; }

# ─── Command-line options ─────────────────────────────────────
INSTALL_MODE_ARG=""
AUTO_YES=false
INSTALL_RISSET=false

usage() {
    cat <<EOF
Usage: ${0##*/} [OPTIONS]

Installs csound7 (static build, no dependencies, glibc>=2.2.5, avx2)

Options:
  --user    Install for the current user only (installs csound in ~/.local/csound, 
            plugins in ~/.local/lib/csound/7.0/plugins64)
  --system  Install system-wide (/usr/local, requires sudo)
  --risset  Also install risset (csound package manager) via uv
  -y        Answer yes to all yes/no questions
  --help    Show this help message and exit
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --user)
            INSTALL_MODE_ARG="user"
            shift
            ;;
        --system)
            INSTALL_MODE_ARG="system"
            shift
            ;;
        -y)
            AUTO_YES=true
            shift
            ;;
        --risset)
            INSTALL_RISSET=true
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ -n "$INSTALL_MODE_ARG" ]]; then
    INSTALL_MODE="$INSTALL_MODE_ARG"
fi

# ─── Helpers ──────────────────────────────────────────────────
ask_yes_no() {
    local prompt="$1" response
    if [ "$AUTO_YES" = true ]; then
        return 0
    fi
    while true; do
        read -rp "$prompt [y/N]: " response
        case "$response" in
            [Yy]* ) return 0 ;;
            [Nn]* | "" ) return 1 ;;
            * ) echo "Please answer yes or no." ;;
        esac
    done
}

ask_choice() {
    local prompt="$1"
    local response
    while true; do
        read -rp "$prompt [(u)ser / (s)ystem]: " response
        case "$response" in
            [Uu]* | user | USER ) echo "user"; return ;;
            [Ss]* | system | SYSTEM ) echo "system"; return ;;
            * ) echo "Please enter 'user' (or 'u') / 'system' (or 's')." ;;
        esac
    done
}

command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# ─── risset installation ──────────────────────────────────────
install_risset() {
    # risset is a python package, installed via uv
    if ! command_exists uv; then
        info "uv is not installed, installing it first..."
        if ! command_exists curl; then
            error "curl is required to install uv but is not installed."
            error "Please install curl, or install uv manually: https://docs.astral.sh/uv/"
            return 1
        fi
        curl -LsSf https://astral.sh/uv/install.sh | sh
        # Make uv available in this session (the installer puts it in ~/.local/bin)
        export PATH="$HOME/.local/bin:$PATH"
        if ! command_exists uv; then
            error "uv installation failed."
            return 1
        fi
        ok "uv installed: $(uv --version)"
    fi
    info "Installing risset..."
    uv tool install risset
    ok "risset installed. Run 'risset --help' to get started."
}

# ─── Detect shell ─────────────────────────────────────────────
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

# ─── Shell-specific config generators ─────────────────────────
path_config_for_shell() {
    local dir="$1" shell_name="$2"
    case "$shell_name" in
        fish)
            echo "fish_add_path $dir"
            ;;
        *)
            echo "export PATH=\"$dir:\$PATH\""
            ;;
    esac
}

# Remove existing PATH entries for our directory from shell config
remove_path_from_config() {
    local rc_file="$1" dir="$2" shell_name="$3"
    if [ ! -f "$rc_file" ]; then return; fi
    case "$shell_name" in
        fish)
            sed -i.bak "/fish_add_path.*$(echo "$dir" | sed 's/[\/&]/\\&/g')/d" "$rc_file" && rm -f "$rc_file.bak"
            ;;
        *)
            sed -i.bak "/$(echo "$dir" | sed 's/[\/&]/\\&/g')/d" "$rc_file" && rm -f "$rc_file.bak"
            ;;
    esac
}

path_already_in_config() {
    local rc_file="$1" dir="$2" shell_name="$3"
    if [ ! -f "$rc_file" ]; then return 1; fi
    case "$shell_name" in
        fish)
            grep -q "fish_add_path.*$dir" "$rc_file" 2>/dev/null
            ;;
        *)
            grep -q "$dir" "$rc_file" 2>/dev/null
            ;;
    esac
}

# ─── Determine script location ────────────────────────────────
# The zip is extracted; we need to find where this script lives
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
info "Source directory: $SCRIPT_DIR"

# ─── Validate source files ────────────────────────────────────
CSOUND_BIN="$SCRIPT_DIR/csound"
LIB_NAME="libcsound64.so.7.0"
LIB_FILE="$SCRIPT_DIR/$LIB_NAME"
PLUGINS_DIR="$SCRIPT_DIR/plugins"

if [ ! -f "$CSOUND_BIN" ]; then
    error "csound executable not found in $SCRIPT_DIR"
    exit 1
fi

if [ ! -f "$LIB_FILE" ]; then
    error "$LIB_NAME not found in $SCRIPT_DIR"
    exit 1
fi

# ─── Choose installation mode ─────────────────────────────────
header ""
header "═══════════════════════════════════════════════════════"
header "  Csound 7 Portable Installer"
header "═══════════════════════════════════════════════════════"
echo ""
echo "  Install for:"
echo "    (u)ser   - Current user only (~/.local)"
echo "    (s)ystem - All users (/usr/local, requires sudo)"
echo ""

if [ -z "${INSTALL_MODE:-}" ]; then
    INSTALL_MODE=$(ask_choice "Installation mode")
fi

# ─── System-wide installation ─────────────────────────────────
if [ "$INSTALL_MODE" = "system" ]; then

    header ""
    header "System-wide installation selected"
    header ""

    PREFIX="/usr/local"
    BIN_DIR="$PREFIX/bin"
    LIB_DIR="$PREFIX/lib"
    PLUGIN_DIR="$PREFIX/lib/csound/plugins64-7.0"

    # Check for sudo
    if [ "$EUID" -ne 0 ]; then
        if command_exists sudo; then
            SUDO="sudo"
        else
            error "System installation requires root privileges. Please run as root or install sudo."
            exit 1
        fi
    else
        SUDO=""
    fi

    # Check for conflicts
    CONFLICT=false
    if [ -f "$BIN_DIR/csound" ]; then
        warn "csound already exists at $BIN_DIR/csound"
        CONFLICT=true
    fi
    if [ -f "$LIB_DIR/$LIB_NAME" ]; then
        warn "$LIB_NAME already exists at $LIB_DIR/$LIB_NAME"
        CONFLICT=true
    fi
    if [ -d "$PLUGIN_DIR" ] && [ "$(ls -A "$PLUGIN_DIR" 2>/dev/null)" ]; then
        warn "Plugin directory already exists and is not empty: $PLUGIN_DIR"
        CONFLICT=true
    fi

    if [ "$CONFLICT" = true ]; then
        if ! ask_yes_no "One or more target files/directories already exist. Overwrite?"; then
            info "Installation cancelled."
            exit 0
        fi
    fi

    # Check for patchelf
    if ! command_exists patchelf; then
        error "patchelf is required for system installation but not installed."
        error "Please install it (e.g., sudo apt install patchelf)"
        exit 1
    fi

    # Create temp copy to patch
    TMP_DIR=$(mktemp -d)
    trap 'rm -rf "$TMP_DIR"' EXIT

    info "Patching rpath for system layout..."
    cp "$CSOUND_BIN" "$TMP_DIR/csound"
    # Change rpath from $ORIGIN to $ORIGIN/../lib (bin is sibling to lib)
    patchelf --set-rpath '$ORIGIN/../lib' "$TMP_DIR/csound"
    ok "rpath patched: \$ORIGIN/../lib"

    # Install
    info "Installing to $PREFIX..."

    $SUDO mkdir -p "$BIN_DIR"
    $SUDO mkdir -p "$LIB_DIR"
    $SUDO mkdir -p "$PLUGIN_DIR"

    $SUDO cp "$TMP_DIR/csound" "$BIN_DIR/csound"
    $SUDO chmod +x "$BIN_DIR/csound"

    $SUDO cp "$LIB_FILE" "$LIB_DIR/$LIB_NAME"
    $SUDO ln -sf "$LIB_DIR/$LIB_NAME" "$LIB_DIR/libcsound64.so"

    if [ -d "$PLUGINS_DIR" ]; then
        $SUDO cp -a "$PLUGINS_DIR/." "$PLUGIN_DIR/"
    fi

    # Update ldconfig
    info "Updating library cache..."
    $SUDO ldconfig

    ok "System-wide installation complete!"
    echo ""
    echo "  csound          → $BIN_DIR/csound"
    echo "  $LIB_NAME       → $LIB_DIR/$LIB_NAME"
    echo "  libcsound64.so  → $LIB_DIR/libcsound64.so (symlink)"
    echo "  plugins         → $PLUGIN_DIR"
    echo ""
    echo "  Run 'csound --version' to verify."

# ─── User-local installation ──────────────────────────────────
else

    header ""
    header "User-local installation selected"
    header ""

    INSTALL_DIR="$HOME/.local/csound"
    PLUGIN_DIR="$HOME/.local/lib/csound/7.0/plugins64"

    # Check for conflicts
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
        if ! ask_yes_no "One or more target directories already exist. Overwrite?"; then
            info "Installation cancelled."
            exit 0
        fi
    fi

    # Install
    info "Installing to $INSTALL_DIR..."

    rm -rf "$INSTALL_DIR"
    mkdir -p "$INSTALL_DIR"

    cp "$CSOUND_BIN" "$INSTALL_DIR/csound"
    chmod +x "$INSTALL_DIR/csound"

    cp "$LIB_FILE" "$INSTALL_DIR/$LIB_NAME"
    ln -sf "$INSTALL_DIR/$LIB_NAME" "$INSTALL_DIR/libcsound64.so"

    rm -rf "$PLUGIN_DIR"
    mkdir -p "$PLUGIN_DIR"

    if [ -d "$PLUGINS_DIR" ]; then
        cp -a "$PLUGINS_DIR/." "$PLUGIN_DIR/"
    fi

    ok "Files installed."

    # Update PATH in shell config
    SHELL_NAME=$(detect_shell)
    SHELL_RC=$(detect_shell_rc "$SHELL_NAME")

    info "Detected shell: $SHELL_NAME"
    info "Shell config: $SHELL_RC"

    # Ensure fish config directory exists
    if [ "$SHELL_NAME" = "fish" ]; then
        mkdir -p "$(dirname "$SHELL_RC")"
    fi

    PATH_LINE=$(path_config_for_shell "$INSTALL_DIR" "$SHELL_NAME")

    # Remove old entries first to avoid duplicates
    remove_path_from_config "$SHELL_RC" "$INSTALL_DIR" "$SHELL_NAME"

    echo "" >> "$SHELL_RC"
    echo "# Added by Csound 7 installer" >> "$SHELL_RC"
    echo "$PATH_LINE" >> "$SHELL_RC"

    ok "PATH updated in $SHELL_RC"

    # Summary
    echo ""
    echo "═══════════════════════════════════════════════════════"
    echo "  User-local installation complete!"
    echo "═══════════════════════════════════════════════════════"
    echo ""
    echo "  csound          → $INSTALL_DIR/csound"
    echo "  $LIB_NAME       → $INSTALL_DIR/$LIB_NAME"
    echo "  libcsound64.so  → $INSTALL_DIR/libcsound64.so (symlink)"
    echo "  plugins         → $PLUGIN_DIR"
    echo ""
    echo "  Shell config:    $SHELL_RC"
    echo ""
    echo "  To use Csound immediately, run:"
    echo "    source $SHELL_RC"
    echo ""
    echo "  Or open a new terminal session."
    echo ""
    echo "  Test with:"
    echo "    csound --version"
    echo "═══════════════════════════════════════════════════════"

fi

# ─── Optional: install risset ─────────────────────────────────
echo ""
if [ "$INSTALL_RISSET" = true ] || ask_yes_no "Install risset (csound package manager)?"; then
    install_risset || warn "risset installation failed. You can retry later with: uv tool install risset"
fi

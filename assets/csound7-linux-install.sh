#!/usr/bin/env bash
set -euo pipefail

# ═══════════════════════════════════════════════════════════════
# Csound 7 Portable Linux Installer (interactive)
#
# This is the interactive installer, it must be run from a terminal
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

debug() {
    if [ "$VERBOSE" = true ]; then
        printf "[debug] $*"
    fi
}

# ─── Command-line options ─────────────────────────────────────
VERBOSE=false
USER_CSOUND_PATH="$HOME/.local/csound"
USER_PLUGINS_DIR="$HOME/.local/lib/csound/7.0/plugins64"

usage() {
    cat <<EOF
Usage: ${0##*/} [OPTIONS]

Installs csound7 (static build, no dependencies, glibc>=2.2.5, avx2)

This installer is interactive and must be run from a terminal.

Options:
  --verbose    Output extra information
  --help       Show this help message and exit
EOF
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --verbose)
            VERBOSE=true
            shift
            ;;
        --help)
            usage
            exit 0
            ;;
        *)
            error "Unknown option: $1"
            exit 1
            ;;
    esac
done

if [ ! -t 0 ]; then
    error "This installer must be run from a terminal."
    exit 1
fi

# ─── Helpers ──────────────────────────────────────────────────
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


install_externals() {
    mkdir -p "$USER_PLUGINS_DIR"
    cp -a "$SOURCE_EXTERNALS/." "$USER_PLUGINS_DIR/"
    info "Installing external plugins to $USER_PLUGINS_DIR"
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
            # If we don't use --global then the change remains across sessions
            # making it rather difficult to remove (it needs to be removed from fish_variables)
            echo "fish_add_path --global $dir"
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
debug "Source directory: $SCRIPT_DIR"

# ─── Validate source files ────────────────────────────────────
CSOUND_BIN="$SCRIPT_DIR/csound"
LIB_NAME="libcsound64.so.7.0"
LIB_FILE="$SCRIPT_DIR/$LIB_NAME"
SOURCE_PLUGINS_DIR="$SCRIPT_DIR/plugins"
SOURCE_EXTERNALS="$SCRIPT_DIR/external-plugins"

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

INSTALL_MODE=$(ask_choice "Installation mode")

# ─── Check for an existing Csound installation ────────────────
if command_exists csound; then
    EXISTING_CSOUND=$(command -v csound)
    USER_CSOUND="$USER_CSOUND_PATH/csound"

    warn "csound is already available at $EXISTING_CSOUND"

    if [ "$INSTALL_MODE" = "user" ] && [ "$EXISTING_CSOUND" = "$USER_CSOUND" ]; then
        info "The existing csound is the user-local installation target."
    elif ! ask_yes_no "A new installation might conflict with the existing csound. Proceed?"; then
        info "Installation cancelled."
        exit 0
    fi
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

    if [ -d "$SOURCE_PLUGINS_DIR" ]; then
        $SUDO cp -a "$SOURCE_PLUGINS_DIR/." "$PLUGIN_DIR/"
    fi

    # Update ldconfig
    info "Updating library cache..."
    $SUDO ldconfig

    ok "System-wide installation complete!"
    echo ""
    echo "  csound          → $BIN_DIR/csound"
    echo "  libcsound64.so  → $LIB_DIR/libcsound64.so (symlink)"
    echo "  plugins         → $PLUGIN_DIR"
    echo ""
    echo "  To uninstall, run:"
    echo "    sudo rm -f $BIN_DIR/csound"
    echo "    sudo rm -f $LIB_DIR/$LIB_NAME $LIB_DIR/libcsound64.so"
    echo "    sudo rm -rf $PLUGIN_DIR"
    echo "    sudo ldconfig"

# ─── User-local installation ──────────────────────────────────
else

    header ""
    header "User-local installation selected"
    header ""

    INSTALL_DIR="$USER_CSOUND_PATH"
    PLUGIN_DIR="$USER_PLUGINS_DIR"

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

    if [ -d "$SOURCE_PLUGINS_DIR" ]; then
        cp -a "$SOURCE_PLUGINS_DIR/." "$PLUGIN_DIR/"
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
    echo "  plugins         → $PLUGIN_DIR"
    echo "  libcsound64.so  → $INSTALL_DIR/libcsound64.so"
    echo ""
    echo "  Shell config:    $SHELL_RC"
    echo ""
    echo "  To use Csound immediately, open a new terminal session or run:"
    echo "    source $SHELL_RC"
    echo ""
    echo "═══════════════════════════════════════════════════════"
    echo ""
    echo "  To uninstall, run:"
    echo "    rm -rf $INSTALL_DIR"
    echo "    rm -rf $PLUGIN_DIR"
    echo "  And remove the '# Added by Csound 7 installer' block"
    echo "  (2 lines) from $SHELL_RC"

fi

# ─── Optional: install externals ─────────────────────────────────
echo ""
if ask_yes_no "Install external plugins?"; then
    install_externals || warn "External plugins installation failed. You can install them via risset"
fi

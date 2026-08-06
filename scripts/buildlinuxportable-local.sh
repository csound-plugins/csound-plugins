#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_ROOT="${BUILD_ROOT:-$ROOT/local-build/linux-portable}"
DEPS_PREFIX="${DEPS_PREFIX:-$BUILD_ROOT/static-deps}"
DIST="$BUILD_ROOT/dist"
ARCHIVE="$BUILD_ROOT/csound7-linux-portable-with-plugins.zip"
JOBS="${JOBS:-$(nproc)}"

LIBOGG_VERSION=1.3.5
LIBVORBIS_VERSION=1.3.7
LIBOPUS_VERSION=1.4
LIBFLAC_VERSION=1.4.3
LIBSNDFILE_VERSION=1.2.2
LIBLO_VERSION=0.35
PORTMIDI_VERSION=2.0.8
PORTAUDIO_VERSION=19.7.0

usage() {
    cat <<EOF
Usage: ${0##*/} [--skip-system-deps] [--skip-static-deps]

Locally reproduces the buildlinuxportable job and packages the portable
distribution into $ARCHIVE.

Environment:
  BUILD_ROOT  Output directory (default: local-build/linux-portable)
  DEPS_PREFIX Static dependency prefix (default: BUILD_ROOT/static-deps)
  JOBS        Parallel build jobs (default: nproc)
EOF
}

install_system_deps=true
build_static_deps=true
while (($#)); do
    case "$1" in
        --skip-system-deps) install_system_deps=false ;;
        --skip-static-deps) build_static_deps=false ;;
        --help) usage; exit 0 ;;
        *) echo "Unknown option: $1" >&2; usage >&2; exit 1 ;;
    esac
    shift
done

require() {
    command -v "$1" >/dev/null || {
        echo "Required command not found: $1" >&2
        exit 1
    }
}

if "$install_system_deps"; then
    sudo apt-get update -qq
    sudo apt-get install -y --no-install-recommends \
        build-essential ca-certificates curl flex bison git nasm php-cli \
        pkg-config xz-utils zip unzip patchelf libasound2-dev libjack-jackd2-dev
fi

for command in ar cmake curl make patchelf pkg-config tar unzip zip; do
    require "$command"
done

git -C "$ROOT" submodule update --init --recursive
mkdir -p "$BUILD_ROOT/sources"

cmake_build() {
    local name="$1"
    local source="$2"
    shift 2
    cmake -B "$BUILD_ROOT/$name-build" -S "$source" "$@"
    cmake --build "$BUILD_ROOT/$name-build" -j"$JOBS"
    cmake --install "$BUILD_ROOT/$name-build"
}

if "$build_static_deps"; then
    rm -rf "$DEPS_PREFIX"
    mkdir -p "$DEPS_PREFIX"

    curl -fsSL "https://downloads.xiph.org/releases/ogg/libogg-$LIBOGG_VERSION.tar.gz" |
        tar -xz -C "$BUILD_ROOT/sources"
    cmake_build ogg "$BUILD_ROOT/sources/libogg-$LIBOGG_VERSION" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DINSTALL_DOCS=OFF

    curl -fsSL "https://downloads.xiph.org/releases/vorbis/libvorbis-$LIBVORBIS_VERSION.tar.gz" |
        tar -xz -C "$BUILD_ROOT/sources"
    cmake_build vorbis "$BUILD_ROOT/sources/libvorbis-$LIBVORBIS_VERSION" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" -DCMAKE_PREFIX_PATH="$DEPS_PREFIX" \
        -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON

    curl -fsSL "https://downloads.xiph.org/releases/opus/opus-$LIBOPUS_VERSION.tar.gz" |
        tar -xz -C "$BUILD_ROOT/sources"
    cmake_build opus "$BUILD_ROOT/sources/opus-$LIBOPUS_VERSION" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DOPUS_BUILD_PROGRAMS=OFF \
        -DOPUS_BUILD_TESTING=OFF

    curl -fsSL "https://downloads.xiph.org/releases/flac/flac-$LIBFLAC_VERSION.tar.xz" |
        tar -xJ -C "$BUILD_ROOT/sources"
    cmake_build flac "$BUILD_ROOT/sources/flac-$LIBFLAC_VERSION" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" -DCMAKE_PREFIX_PATH="$DEPS_PREFIX" \
        -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DWITH_OGG=ON \
        -DBUILD_DOCS=OFF -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DINSTALL_MANPAGES=OFF

    curl -fsSL "https://github.com/libsndfile/libsndfile/releases/download/$LIBSNDFILE_VERSION/libsndfile-$LIBSNDFILE_VERSION.tar.xz" |
        tar -xJ -C "$BUILD_ROOT/sources"
    cmake_build sndfile "$BUILD_ROOT/sources/libsndfile-$LIBSNDFILE_VERSION" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" -DCMAKE_PREFIX_PATH="$DEPS_PREFIX" \
        -DBUILD_SHARED_LIBS=OFF -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_PROGRAMS=OFF \
        -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF -DENABLE_EXTERNAL_LIBS=ON -DENABLE_MPEG=OFF

    libdir="$DEPS_PREFIX/lib"
    printf 'CREATE %s/libsndfile_merged.a\nADDLIB %s/libsndfile.a\nADDLIB %s/libFLAC.a\nADDLIB %s/libvorbisenc.a\nADDLIB %s/libvorbis.a\nADDLIB %s/libogg.a\nADDLIB %s/libopus.a\nSAVE\nEND\n' \
        "$libdir" "$libdir" "$libdir" "$libdir" "$libdir" "$libdir" "$libdir" > "$BUILD_ROOT/merge.mri"
    ar -M < "$BUILD_ROOT/merge.mri"
    mv "$libdir/libsndfile_merged.a" "$libdir/libsndfile.a"

    curl -fsSL "https://github.com/PortMidi/portmidi/archive/refs/tags/v$PORTMIDI_VERSION.tar.gz" |
        tar -xz -C "$BUILD_ROOT/sources"
    cmake_build portmidi "$BUILD_ROOT/sources/portmidi-$PORTMIDI_VERSION" \
        -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" \
        -DBUILD_SHARED_LIBS=OFF -DBUILD_PORTMIDI_TESTS=OFF

    curl -fsSL "https://github.com/PortAudio/portaudio/archive/refs/tags/v$PORTAUDIO_VERSION.tar.gz" |
        tar -xz -C "$BUILD_ROOT/sources"
    cmake_build portaudio "$BUILD_ROOT/sources/portaudio-$PORTAUDIO_VERSION" \
        -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$DEPS_PREFIX" -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DPA_BUILD_STATIC=ON \
        -DPA_BUILD_SHARED=OFF -DPA_USE_ALSA=ON -DPA_ALSA_DYNAMIC=OFF -DPA_USE_JACK=ON

    curl -fsSL "https://github.com/radarsat1/liblo/releases/download/$LIBLO_VERSION/liblo-$LIBLO_VERSION.tar.gz" |
        tar -xz -C "$BUILD_ROOT/sources"
    (
        cd "$BUILD_ROOT/sources/liblo-$LIBLO_VERSION"
        ./configure --prefix="$DEPS_PREFIX" --enable-static --disable-shared \
            --disable-examples --disable-tools --disable-doc CFLAGS="-fPIC -O2"
        make -j"$JOBS"
        make install
    )
fi

PKG_CONFIG_PATH="$DEPS_PREFIX/lib/pkgconfig" cmake -B "$BUILD_ROOT/csound-build" -S "$ROOT/csound" \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$DEPS_PREFIX" \
    -DPortMidi_DIR="$DEPS_PREFIX/lib/cmake/PortMidi" \
    -DCMAKE_C_FLAGS="-march=x86-64-v3" -DCMAKE_CXX_FLAGS="-march=x86-64-v3" \
    -DCMAKE_EXE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -DCMAKE_SHARED_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -DCMAKE_MODULE_LINKER_FLAGS="-static-libgcc -static-libstdc++" \
    -DCMAKE_PROJECT_TOP_LEVEL_INCLUDES="$ROOT/.github/cmake/linux-portable.cmake" \
    -DBUILD_PLUGINS=ON -DUSE_AVX2=ON -DUSE_LIBLO=ON -DUSE_ALSA=ON -DUSE_JACK=ON \
    -DUSE_PIPEWIRE=OFF -DUSE_PORTMIDI=ON -DUSE_PORTAUDIO=ON -DUSE_PULSEAUDIO=OFF \
    -DBUILD_UTILITIES=OFF \
    -DUSE_FLTK=OFF -DBUILD_VIRTUAL_KEYBOARD=OFF -DBUILD_JAVA_INTERFACE=OFF \
    -DBUILD_PYTHON_INTERFACE=OFF -DINSTALL_PYTHON_INTERFACE=OFF -DBUILD_LUA_INTERFACE=OFF \
    -DBUILD_CSBEATS=OFF -DBUILD_TESTS=OFF -DCMAKE_INSTALL_RPATH='$ORIGIN' \
    -DCMAKE_BUILD_WITH_INSTALL_RPATH=ON
cmake --build "$BUILD_ROOT/csound-build" -j"$JOBS"

! ldd "$BUILD_ROOT/csound-build/libpmidi.so" | grep -F 'libportmidi.so'
! ldd "$BUILD_ROOT/csound-build/librtpa.so" | grep -F 'libportaudio.so'
ldd "$BUILD_ROOT/csound-build/librtpa.so" | grep -F 'libasound.so'
ldd "$BUILD_ROOT/csound-build/librtpa.so" | grep -F 'libjack.so'
! ldd "$BUILD_ROOT/csound-build/libemugens.so" | grep -E 'lib(pipewire|asound|jack)\.so'

cmake -B "$BUILD_ROOT/plugins-build" -S "$ROOT" -DAPIVERSION=7.0 \
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_PREFIX_PATH="$DEPS_PREFIX" \
    -DCMAKE_FIND_LIBRARY_SUFFIXES=".a" -DCMAKE_C_FLAGS="-march=x86-64-v3" \
    -DCMAKE_CXX_FLAGS="-march=x86-64-v3" \
    -DCMAKE_MODULE_LINKER_FLAGS="-static-libgcc -static-libstdc++"
cmake --build "$BUILD_ROOT/plugins-build" -j"$JOBS"

rm -rf "$DIST" "$ARCHIVE"
mkdir -p "$DIST/plugins" "$DIST/external-plugins"
cp "$BUILD_ROOT/csound-build/csound" "$DIST/"
find "$BUILD_ROOT/csound-build" -name 'libcsound*.so*' -exec cp -P {} "$DIST/" \;
find "$BUILD_ROOT/csound-build" -name '*.so' ! -name 'libcsound*' -exec cp {} "$DIST/plugins/" \;
find "$BUILD_ROOT/plugins-build" -maxdepth 1 -type f -name '*.so' -exec cp {} "$DIST/external-plugins/" \;
cp "$ROOT/assets/README.txt" "$DIST/plugins/"
cp "$ROOT/assets/csound7-linux-install.sh" "$DIST/install.sh"
chmod a+rx "$DIST/install.sh" "$DIST/csound" "$DIST/libcsound64.so.7.0"
patchelf --set-rpath '$ORIGIN' "$DIST/csound"
(
    cd "$DIST"
    zip -r "$ARCHIVE" .
)

echo "Portable archive created: $ARCHIVE"

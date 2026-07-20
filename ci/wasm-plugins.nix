{
  pkgs ? import <nixpkgs> {},
  pkgsWasm ? pkgs.pkgsCross.wasi32,
  stdenvWasm ? pkgsWasm.clangStdenv,
  csoundSdkArchive,
}:

stdenvWasm.mkDerivation {
  pname = "csound-plugins-wasm";
  version = "0.0.0";
  src = ../src;

  nativeBuildInputs = [ pkgs.buildPackages.gnutar ];
  dontConfigure = true;
  dontStrip = true;

  buildPhase = ''
    tar -xzf ${csoundSdkArchive}
    sdk="$PWD/csound-plugin-sdk"
    mkdir -p build

    compile_plugin() {
      name="$1"
      source="$2"

      $CC \
        -fPIC \
        -mllvm -wasm-enable-sjlj \
        -D__wasi__=1 \
        -D__wasm32__=1 \
        -D_WASI_EMULATED_SIGNAL \
        -D_WASI_EMULATED_MMAN \
        -DUSE_DOUBLE=1 \
        -I"$sdk/include" \
        -I"$sdk/include/csound" \
        -I"$src/common" \
        -c "$source" \
        -o "build/$name.o"

      $CC \
        -Wl,-z,stack-size=131072 \
        -Wl,--import-table \
        -Wl,--import-memory \
        -Wl,--export-all \
        -Wl,--no-entry \
        -lwasi-emulated-signal \
        -lwasi-emulated-mman \
        "build/$name.o" \
        -lm \
        -o "build/$name.wasm"
    }

    compile_plugin else "$src/else/src/else.c"
    compile_plugin klib "$src/klib/src/klib.c"
  '';

  installPhase = ''
    mkdir -p "$out/lib"
    cp build/else.wasm build/klib.wasm "$out/lib/"
  '';
}

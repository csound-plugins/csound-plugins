option(BUILD_SEMSYS_OPCODES "Build semsys opcodes (Csound API 7; onnxruntime + onnxruntime-extensions needed at runtime)" ON)

if(BUILD_SEMSYS_OPCODES)
    # Soft gate: semsys is Csound API 7 only. Skip quietly on API 6 so the other
    # plugins still configure and build.
    if(DEFINED APIVERSION AND NOT APIVERSION MATCHES "^7\\.0$")
        message(STATUS "semsys: skipped (requires Csound API 7, got APIVERSION=${APIVERSION}; configure with -DAPIVERSION=7.0)")
        return()
    endif()

    # onnxruntime is loaded at runtime via dlopen (see load_ort_get_api_base in
    # semsys.c): the C API is a function-pointer table reached from the single symbol
    # OrtGetApiBase. So building needs ONLY the vendored C API headers in
    # third_party/onnxruntime -- no onnxruntime install, no link dependency, no
    # find_package. The shared library is required at runtime, not at build time.
    make_plugin(semsys src/semsys.c)
    target_include_directories(semsys PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/onnxruntime")
    target_link_libraries(semsys PRIVATE ${CMAKE_DL_LIBS})   # dlopen/dlsym/dladdr (empty on win/mac)

    # rpath so the bundled sibling libraries (below) resolve from the plugin's own dir
    if(APPLE)
        set_target_properties(semsys PROPERTIES BUILD_RPATH "@loader_path" INSTALL_RPATH "@loader_path")
    elseif(UNIX)
        set_target_properties(semsys PROPERTIES BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")
    endif()

    # --- optional runtime bundling ---------------------------------------------
    # The build does not need these; a self-contained plugin zip does. When the
    # runtime shared libraries are provided (or discoverable) they are copied next to
    # the plugin and installed. Otherwise supply them at runtime via the
    # SEMSYS_ONNXRUNTIME / SEMSYS_ORT_EXTENSIONS env vars, or a system install.
    if(WIN32)
        set(_onnx_name "onnxruntime.dll")
        set(_ortext_name "ortextensions.dll")
    elseif(APPLE)
        set(_onnx_name "libonnxruntime.dylib")
        set(_ortext_name "libortextensions.dylib")
    else()
        set(_onnx_name "libonnxruntime.so")
        set(_ortext_name "libortextensions.so")
    endif()

    # onnxruntime runtime library: -DONNXRUNTIME_LIB=/path (explicit) wins, else
    # discovered under -DONNXRUNTIME_ROOT / env ONNXRUNTIME_ROOT, else system paths.
    if(NOT ONNXRUNTIME_LIB)
        find_file(ONNXRUNTIME_LIB
            NAMES "${_onnx_name}" onnxruntime.dll libonnxruntime.dylib libonnxruntime.so
            HINTS "${ONNXRUNTIME_ROOT}" ENV ONNXRUNTIME_ROOT
            PATH_SUFFIXES lib lib64 bin
        )
    endif()
    if(ONNXRUNTIME_LIB AND EXISTS "${ONNXRUNTIME_LIB}")
        add_custom_command(TARGET semsys POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${ONNXRUNTIME_LIB}" "$<TARGET_FILE_DIR:semsys>/${_onnx_name}"
            VERBATIM)
        install(FILES "${ONNXRUNTIME_LIB}" DESTINATION "${PLUGIN_INSTALL_DIR}" RENAME "${_onnx_name}")
        message(STATUS "semsys: bundling onnxruntime from ${ONNXRUNTIME_LIB}")
    else()
        message(STATUS "semsys: onnxruntime runtime lib not bundled "
                       "(pass -DONNXRUNTIME_LIB=/path/${_onnx_name} or -DONNXRUNTIME_ROOT=/root). "
                       "Provide libonnxruntime at runtime via SEMSYS_ONNXRUNTIME or a system install.")
    endif()

    # onnxruntime-extensions runtime library (custom ops the E2E embedding/STT graphs
    # need), loaded via RegisterCustomOpsLibrary. -DORT_EXTENSIONS_LIB=/path (explicit)
    # or discovered under -DORTEXTENSIONS_ROOT / env ORTEXTENSIONS_ROOT. Note: the PyPI
    # wheel's native lib (onnxruntime_extensions.get_library_path()) works here too.
    if(NOT ORT_EXTENSIONS_LIB)
        find_library(ORT_EXTENSIONS_LIB
            NAMES ortextensions libortextensions
            HINTS "${ORTEXTENSIONS_ROOT}" ENV ORTEXTENSIONS_ROOT
            PATH_SUFFIXES lib lib64 bin)
    endif()
    if(ORT_EXTENSIONS_LIB AND EXISTS "${ORT_EXTENSIONS_LIB}")
        add_custom_command(TARGET semsys POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${ORT_EXTENSIONS_LIB}" "$<TARGET_FILE_DIR:semsys>/${_ortext_name}"
            VERBATIM)
        install(FILES "${ORT_EXTENSIONS_LIB}" DESTINATION "${PLUGIN_INSTALL_DIR}" RENAME "${_ortext_name}")
        message(STATUS "semsys: bundling onnxruntime-extensions from ${ORT_EXTENSIONS_LIB}")
    else()
        message(STATUS "semsys: onnxruntime-extensions not bundled "
                       "(pass -DORT_EXTENSIONS_LIB=/path/${_ortext_name} or -DORTEXTENSIONS_ROOT=/root). "
                       "Provide it at runtime via SEMSYS_ORT_EXTENSIONS.")
    endif()
endif()

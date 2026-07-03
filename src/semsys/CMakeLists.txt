option(BUILD_SEMSYS_OPCODES "Build semsys opcodes (needs onnxruntime + onnxruntime-extensions, Csound API 7)" ON)

if(BUILD_SEMSYS_OPCODES)
    # Soft gate: build semsys only when its prerequisites are present. If not,
    # skip quietly (message + return) so the other plugins and CI without
    # onnxruntime still configure and build.
    if(DEFINED APIVERSION AND NOT APIVERSION MATCHES "^7\\.0$")
        message(STATUS "semsys: skipped (requires Csound API 7, got APIVERSION=${APIVERSION}; configure with -DAPIVERSION=7.0)")
        return()
    endif()

    if(ONNXRUNTIME_ROOT OR DEFINED ENV{ONNXRUNTIME_ROOT})
        find_package(onnxruntime MODULE)
    else()
        find_package(onnxruntime CONFIG QUIET)
    endif()
    if(NOT onnxruntime_FOUND)
        find_package(onnxruntime QUIET)
    endif()
    if(NOT onnxruntime_FOUND)
        message(STATUS "semsys: skipped (onnxruntime not found; install it or pass -DONNXRUNTIME_ROOT=/path)")
        return()
    endif()

    make_plugin(semsys src/semsys.c)
    target_link_libraries(semsys PRIVATE onnxruntime::onnxruntime)

    if(APPLE)
        set_target_properties(semsys PROPERTIES
            BUILD_RPATH "@loader_path"
            INSTALL_RPATH "@loader_path"
        )
    elseif(UNIX)
        set_target_properties(semsys PROPERTIES
            BUILD_RPATH "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
        )
    endif()

    get_target_property(_onnxruntime_target_type onnxruntime::onnxruntime TYPE)
    if(_onnxruntime_target_type STREQUAL "SHARED_LIBRARY")
        add_custom_command(TARGET semsys POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_FILE:onnxruntime::onnxruntime>"
                "$<TARGET_FILE_DIR:semsys>"
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "$<TARGET_SONAME_FILE:onnxruntime::onnxruntime>"
                "$<TARGET_FILE_DIR:semsys>"
            VERBATIM
        )
        if(APPLE)
            add_custom_command(TARGET semsys POST_BUILD
                COMMAND "${CMAKE_COMMAND}"
                    "-DTARGET_FILE=$<TARGET_FILE:semsys>"
                    "-DDEPENDENCY_FILE=$<TARGET_FILE:onnxruntime::onnxruntime>"
                    "-DDEPENDENCY_NAME=$<TARGET_SONAME_FILE_NAME:onnxruntime::onnxruntime>"
                    -P "${CMAKE_SOURCE_DIR}/cmake/Modules/FixMachODependencyInstallName.cmake"
                VERBATIM
            )
        endif()
        install(IMPORTED_RUNTIME_ARTIFACTS onnxruntime::onnxruntime
            RUNTIME DESTINATION "${PLUGIN_INSTALL_DIR}"
            LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}"
            FRAMEWORK DESTINATION "${PLUGIN_INSTALL_DIR}"
        )
    else()
        message(STATUS "onnxruntime target is ${_onnxruntime_target_type}; runtime library installation is left to the package manager")
    endif()

    # onnxruntime-extensions: the end-to-end embedding and STT graphs use custom ops
    # from this library (the embedded tokenizer, audio decode/STFT, BPE detokenizer),
    # loaded at runtime (RegisterCustomOpsLibrary) and resolved next to the plugin.
    # Provide the NATIVE (non-Python) shared lib in one of two ways:
    #   -DORT_EXTENSIONS_LIB=/full/path/to/libortextensions.<ext>   (explicit), or
    #   -DORTEXTENSIONS_ROOT=/root (or env ORTEXTENSIONS_ROOT)       (discovered below)
    if(WIN32)
        set(_ortext_name "ortextensions.dll")
    elseif(APPLE)
        set(_ortext_name "libortextensions.dylib")
    else()
        set(_ortext_name "libortextensions.so")
    endif()

    if(NOT ORT_EXTENSIONS_LIB)
        find_library(ORT_EXTENSIONS_LIB
            NAMES ortextensions libortextensions
            HINTS "${ORTEXTENSIONS_ROOT}" ENV ORTEXTENSIONS_ROOT
            PATH_SUFFIXES lib lib64 bin
        )
    endif()

    if(ORT_EXTENSIONS_LIB)
        if(NOT EXISTS "${ORT_EXTENSIONS_LIB}")
            message(FATAL_ERROR "ORT_EXTENSIONS_LIB does not exist: ${ORT_EXTENSIONS_LIB}")
        endif()
        add_custom_command(TARGET semsys POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${ORT_EXTENSIONS_LIB}"
                "$<TARGET_FILE_DIR:semsys>/${_ortext_name}"
            VERBATIM
        )
        install(FILES "${ORT_EXTENSIONS_LIB}"
            DESTINATION "${PLUGIN_INSTALL_DIR}"
            RENAME "${_ortext_name}"
        )
    else()
        message(STATUS "onnxruntime-extensions (${_ortext_name}) not found; it will NOT be bundled. "
                       "The semsys embedding and STT models need it at runtime: pass -DORT_EXTENSIONS_LIB=/path/to/${_ortext_name}, "
                       "or -DORTEXTENSIONS_ROOT=/root (env ORTEXTENSIONS_ROOT), "
                       "or set the SEMSYS_ORT_EXTENSIONS env var at runtime.")
    endif()
endif()

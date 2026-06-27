option(BUILD_SEMSYS_OPCODES "Build semsys opcodes" ON)

if(BUILD_SEMSYS_OPCODES)
    if(DEFINED APIVERSION AND NOT APIVERSION MATCHES "^7\\.0$")
        message(FATAL_ERROR "semsys requires Csound API 7; configure with -DAPIVERSION=7.0")
    endif()

    if(ONNXRUNTIME_ROOT OR DEFINED ENV{ONNXRUNTIME_ROOT})
        find_package(onnxruntime REQUIRED MODULE)
    else()
        find_package(onnxruntime CONFIG QUIET)
    endif()
    if(NOT onnxruntime_FOUND)
        find_package(onnxruntime REQUIRED)
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
endif()

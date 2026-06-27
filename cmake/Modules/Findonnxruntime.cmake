include(FindPackageHandleStandardArgs)

set(_ONNXRUNTIME_HINTS)
if(ONNXRUNTIME_ROOT)
    list(APPEND _ONNXRUNTIME_HINTS "${ONNXRUNTIME_ROOT}")
endif()
if(DEFINED ENV{ONNXRUNTIME_ROOT})
    list(APPEND _ONNXRUNTIME_HINTS "$ENV{ONNXRUNTIME_ROOT}")
endif()

find_path(ONNXRUNTIME_INCLUDE_DIR
    NAMES onnxruntime_c_api.h
    HINTS ${_ONNXRUNTIME_HINTS}
    PATH_SUFFIXES include include/onnxruntime
)

find_library(ONNXRUNTIME_LIBRARY
    NAMES onnxruntime libonnxruntime
    HINTS ${_ONNXRUNTIME_HINTS}
    PATH_SUFFIXES lib lib64
)

if(WIN32)
    find_file(ONNXRUNTIME_RUNTIME_LIBRARY
        NAMES onnxruntime.dll
        HINTS ${_ONNXRUNTIME_HINTS}
        PATH_SUFFIXES bin lib
    )
else()
    set(ONNXRUNTIME_RUNTIME_LIBRARY "${ONNXRUNTIME_LIBRARY}")
endif()

find_package_handle_standard_args(onnxruntime
    REQUIRED_VARS
        ONNXRUNTIME_INCLUDE_DIR
        ONNXRUNTIME_LIBRARY
        ONNXRUNTIME_RUNTIME_LIBRARY
)

if(onnxruntime_FOUND AND NOT TARGET onnxruntime::onnxruntime)
    add_library(onnxruntime::onnxruntime SHARED IMPORTED)
    set_target_properties(onnxruntime::onnxruntime PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${ONNXRUNTIME_INCLUDE_DIR}"
        IMPORTED_LOCATION "${ONNXRUNTIME_RUNTIME_LIBRARY}"
    )
    if(WIN32)
        set_target_properties(onnxruntime::onnxruntime PROPERTIES
            IMPORTED_IMPLIB "${ONNXRUNTIME_LIBRARY}"
        )
    endif()
endif()

mark_as_advanced(
    ONNXRUNTIME_INCLUDE_DIR
    ONNXRUNTIME_LIBRARY
    ONNXRUNTIME_RUNTIME_LIBRARY
)

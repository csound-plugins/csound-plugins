if(NOT DEFINED TARGET_FILE OR NOT DEFINED DEPENDENCY_FILE OR NOT DEFINED DEPENDENCY_NAME)
    message(FATAL_ERROR "TARGET_FILE, DEPENDENCY_FILE and DEPENDENCY_NAME are required")
endif()

execute_process(
    COMMAND /usr/bin/otool -D "${DEPENDENCY_FILE}"
    OUTPUT_VARIABLE _dependency_id_output
    RESULT_VARIABLE _otool_result
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(NOT _otool_result EQUAL 0)
    message(FATAL_ERROR "Could not read Mach-O install name from ${DEPENDENCY_FILE}")
endif()

string(REPLACE "\n" ";" _dependency_id_lines "${_dependency_id_output}")
list(LENGTH _dependency_id_lines _dependency_id_line_count)
if(_dependency_id_line_count LESS 2)
    message(FATAL_ERROR "Unexpected otool -D output for ${DEPENDENCY_FILE}: ${_dependency_id_output}")
endif()

list(GET _dependency_id_lines 1 _dependency_id)
string(STRIP "${_dependency_id}" _dependency_id)

execute_process(
    COMMAND /usr/bin/install_name_tool
        -change "${_dependency_id}" "@loader_path/${DEPENDENCY_NAME}" "${TARGET_FILE}"
    RESULT_VARIABLE _install_name_tool_result
)

if(NOT _install_name_tool_result EQUAL 0)
    message(FATAL_ERROR "Could not rewrite ${_dependency_id} in ${TARGET_FILE}")
endif()

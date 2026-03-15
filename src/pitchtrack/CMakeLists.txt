option(BUILD_PITCHTRACK_OPCODES "Build pitchtrack plugin" ON)

if(NOT(BUILD_PITCHTRACK_OPCODES))
    return()
endif()

set(pluginlib pitchtrack)
set(SOURCE_FILES src/pyinlib.c src/pyin.c)

if(APPLE)
    add_library(${pluginlib} SHARED ${SOURCE_FILES})
else()
    add_library(${pluginlib} MODULE ${SOURCE_FILES})
endif()

target_include_directories(${pluginlib} PRIVATE ${CMAKE_SOURCE_DIR}/csound/include)
target_include_directories(${pluginlib} PRIVATE ${CMAKE_SOURCE_DIR}/csound/Android/CsoundAndroid/jni)

set_target_properties(${pluginlib} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
  LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
)
install(TARGETS ${pluginlib} LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}")

include(CheckLibraryExists)
CHECK_LIBRARY_EXISTS(m sin "" HAVE_LIB_M)
if (HAVE_LIB_M)
    set(EXTRA_LIBS ${EXTRA_LIBS} m)
endif (HAVE_LIB_M)

target_link_libraries(${pluginlib} PUBLIC ${EXTRA_LIBS})

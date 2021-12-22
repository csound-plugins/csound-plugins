option(BUILD_JSUSFX_OPCODES "Build jsusfx opcodes" ON)

if(NOT(BUILD_JSUSFX_OPCODES))
  return()
endif()

cmake_minimum_required(VERSION 2.7)
project (jsusfx_csound)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_SUPPRESS_REGENERATION true)
set(CMAKE_MACOSX_RPATH Off)
#set(CMAKE_OSX_DEPLOYMENT_TARGET 10.7)
set(CMAKE_OSX_ARCHITECTURES "i386;x86_64")

# add_subdirectory(${PROJECT_SOURCE_DIR}/../src ${PROJECT_SOURCE_DIR}/../src)

# add_subdirectory(${PROJECT_SOURCE_DIR}/jsfx-src ${PROJECT_SOURCE_DIR}/jsfx-src)
# add_subdirectory(jsfx-src jsfx-src)
add_subdirectory(jsusfx/src)


include(csound-plugins.cmake)

# include_directories(${PROJECT_SOURCE_DIR}/jsfx-src)
include_directories($(PROJECT_SOURCE_DIR)/jsusfx/src)

# These functions are defined in csound-plugins.cmake
# The variables are defined in the root CMakeLists.txt (../../CmakeLists.txt)
# set_csound_sources(${CSOUND_INCLUDE_DIRS})
message(STATUS "csound sources: ${PROJECT_SOURCE_DIR}/../../csound/include")
set_csound_sources(${PROJECT_SOURCE_DIR}/../../csound/include)
set_csound_build_path(${BUILD_PLUGINS_DIR})
add_csound_plugin(jsusfx_csound_project libjsfx csound/jsusfx_csound.cpp)

target_compile_definitions(jsusfx_csound_project PRIVATE USE_DOUBLE)
target_link_libraries(jsusfx_csound_project jsusfx)

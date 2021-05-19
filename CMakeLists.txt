project("csound_plugins")

cmake_minimum_required(VERSION 3.8)

set(APIVERSION "6.0")

# Release or Debug
set(CMAKE_BUILD_TYPE "Release")

# switch RPATH use off
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)

# force make to print the command lines
set(CMAKE_VERBOSE_MAKEFILE on)

# path to Csound cmake module
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# set compilation flags
# set(CMAKE_CXX_FLAGS  ${CMAKE_CXX_FLAGS}  "-std=c++11")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} "-Wall -Wextra -DUSE_DOUBLE -DB64BIT")

# add_definitions(-DUSE_DOUBLE -DB64BIT)

if (MSVC)
    add_compile_options(/W3) #  /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra)
endif()

# options
option(USE_LIB64 "Set to on to set installation dir for libs to lib64" OFF)
option(USE_DOUBLE "Use doubles for audio calculations" ON)
option(CPP11 "c++11" ON)

set(BUILDING_CSOUND_PLUGINS ON)

# ----------------------------------------------

# include(FindCsound)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# -----------------------------------------------

function(addflag flag flagname)
  check_c_compiler_flag(${flag} ${flagname})
  if (${flagname})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}" PARENT_SCOPE)
  endif()
  check_cxx_compiler_flag(${flag} CXX_${flagname})
  if (CXX_${flagname})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
  endif()
endfunction(addflag)


MACRO(SUBDIRLIST result curdir)
  FILE(GLOB children RELATIVE ${curdir} ${curdir}/*)
  SET(dirlist "")
  FOREACH(child ${children})
    message(STATUS "looking at ${child}")
    IF(IS_DIRECTORY ${curdir}/${child})
        LIST(APPEND dirlist ${child})
    ENDIF()
  ENDFOREACH()
  SET(${result} ${dirlist})
ENDMACRO()

  
MACRO(ADD_ALL_SUBDIRECTORIES directory)
  subdirlist(SUBDIRS ${directory})
  message(STATUS "Found subdirs: ${SUBDIRS}")
  foreach(SUBDIR ${SUBDIRS})
    set(thissubdir "${directory}/${SUBDIR}")
    if(EXISTS "${directory}/${SUBDIR}/CMakeLists.txt")
      message(STATUS "Addind subdir: ${thissubdir}")
      add_subdirectory(${directory}/${SUBDIR})
    else()
      message(WARNING "Skipping ${directory}/${SUBDIR} because no CMakeLists.txt file was found")
    endif()
  endforeach()
ENDMACRO()

if(CPP11)
  set(CMAKE_CXX_STANDARD 11)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)    
endif()


# set optimization flags
if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANG)
  add_definitions(-fvisibility=hidden)
  if(NATIVE)
	  add_definitions(-march=native)
  endif()
  
  include(CheckCCompilerFlag)
  include(CheckCXXCompilerFlag)

  addflag(-msse HAS_SSE)
  addflag(-msse2 HAS_SSE2)
  addflag(-mfgpath=sse HAS_FPMATH_SSE)
  
endif()

if(MINGW)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign")
endif()

addflag(-ftree-vectorize HAS_TREE_VECTORIZE)


if(SKIP_FAST_MATH)
  message(STATUS "Skipping -ffast-math")
else()
  # fast-math causes undefined symbols in linux when built inside an azure pipeline
  addflag(-ffast-math HAS_FAST_MATH)          
  message(STATUS "Adding -ffast-math")
endif()

addflag(-fomit-frame-pointer HAS_OMIT_FRAME_POINTER)


# -------------------------------------------------------------------

# User installed plugins should go to ~, since when csound is installed from 
# source, it puts things under ~. However, csound instaled via installer puts
# everything under /. 
set(CS_FRAMEWORK_DEST "~/Library/Frameworks")
# set(CS_FRAMEWORK_DEST "/Library/Frameworks")


if(USE_LIB64)
  set(LIBRARY_INSTALL_DIR "lib64")
  add_definitions("-DLIB64")
else()
  set(LIBRARY_INSTALL_DIR "lib")
endif()

message(STATUS "LIBRARY INSTALL DIR: ${LIBRARY_INSTALL_DIR}")

# -------------------------------------------------------------------


if(USE_DOUBLE)
  message(STATUS ">>> using doubles")
  
  if(APPLE)
    set(CSOUNDLIB "CsoundLib64")
    set(PLUGIN_INSTALL_DIR "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes64")
  else()
    set(CSOUNDLIB "csound64")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins64-${APIVERSION}")
  endif()
else()
  message(STATUS ">>> not using doubles")
  if(APPLE)
    set(CSOUNDLIB "CsoundLib")
    set(PLUGIN_INSTALL_DIR "${CS_FRAMEWORK_DEST}/${CSOUNDLIB}.framework/Versions/${APIVERSION}/Resources/Opcodes")
  else()
    set(CSOUNDLIB "csound")
    set(PLUGIN_INSTALL_DIR "${LIBRARY_INSTALL_DIR}/csound/plugins-${APIVERSION}")
  endif()
endif()


# -------------------------------------------------------------------
  
# Csound opcode build
# find_package(Csound)


set(BUILD_PLUGINS_DIR ${CMAKE_CURRENT_BINARY_DIR})

#if(NOT CSOUND_FOUND)
#  message(FATAL_ERROR "Csound installation not found")
#endif()

# pluginlib: name of the plugin
# srcs: sources of the plugin
function(make_plugin pluginlib srcs)
  message(STATUS "make_plugin: ${pluginlib}")
  if(APPLE)
    add_library(${pluginlib} SHARED ${srcs})
  else()
    add_library(${pluginlib} MODULE ${srcs})
  endif()
  target_include_directories(${pluginlib} PRIVATE ${CMAKE_SOURCE_DIR}/csound/include)
  target_compile_definitions(${pluginlib} PRIVATE USE_DOUBLE)
  # target_include_directories(${pluginlib} PRIVATE ${CSOUND_INCLUDE_DIRS})
  message(STATUS "csound include: ${CMAKE_SOURCE_DIR}/csound/include")
  set_property(TARGET ${pluginlib} PROPERTY C_STANDARD 99)
  set_target_properties(${pluginlib} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${BUILD_PLUGINS_DIR})
  
  install(TARGETS ${pluginlib}
    LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}" )
  
endfunction(make_plugin)


message(STATUS "Building Opcodes")

# ------------------------------------------------------------------------------------

# add_subdirectory(src)

# make_plugin(klib src/klib/klib.c)
# make_plugin(poly src/poly/poly.c)

ADD_ALL_SUBDIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/src)


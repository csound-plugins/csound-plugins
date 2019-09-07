
# The path to this file.
set(CSOUND_CMAKE_PATH ${CMAKE_CURRENT_LIST_DIR})

# The path to csound sources.
set(CSOUND_SOURCES_PATH)

# The output path for the plugins (the build dir)
set(CSOUND_OUTPUT_PATH)


if(WIN32)
  if(NOT CSOUND_LIB_PATH)
    if(${CMAKE_GENERATOR} MATCHES "(Win64|IA64)")
      set(CSOUND_LIB_PATH  ${CSOUND_CMAKE_PATH}/x64)
    else()
      set(CSOUND_LIB_PATH  ${CSOUND_CMAKE_PATH}/x86)
    endif()
  endif()
endif()

# The function adds a plugin to the project.
# PROJECT_NAME is the name of your project (for example: klib_proj)
# PLUGIN_NAME is the name of your plugin (for example: klib)
# PLUGIN_SOURCES are the source files (for example: klib.c)
# The function should be call:
# add_csound_plugin(klibproj klib "userpath/file1.c userpath/file2.c")
# later see how to manage relative and absolute path
function(add_csound_plugin PROJECT_NAME PLUGIN_NAME PLUGIN_SOURCES)
  source_group(src FILES ${PLUGIN_SOURCES})
  if(APPLE)
    add_library(${PROJECT_NAME} SHARED ${PLUGIN_SOURCES})
  else()
    add_library(${PROJECT_NAME} MODULE ${PLUGIN_SOURCES})
  endif()
  
  # Includes the path to csound sources.
  target_include_directories(${PROJECT_NAME} PRIVATE ${CSOUND_SOURCES_PATH})

  # Defines plateform specifix suffix and the linking necessities.
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")

  if(${APPLE})
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dylib")
  elseif(${UNIX})
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".so")
  elseif(${WIN32})
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dll")
    find_library(CSOUND_LIBRARY NAMES csound HINTS ${CSOUND_LIB_PATH})
    target_link_libraries(${PROJECT_NAME} ${CSOUND_LIBRARY})
  endif()

  # Removes some warning for Microsoft Visual C.
  if(${MSVC})
    set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY COMPILE_FLAGS "/wd4091 /wd4996")
  endif()


  # Defines the name of the plugin.
  # On XCode with CMake < 3.4 if the name of an plugin ends with tilde but doesn't have a dot, the name must be 'name~'.
  # CMake 3.4 is not sure, but it should be between 3.3.2 and 3.6.2
  #string(FIND ${PLUGIN_NAME} "." NAME_HAS_DOT)
  #string(FIND ${PLUGIN_NAME} "~" NAME_HAS_TILDE)
  if((${CMAKE_VERSION} VERSION_LESS 3.4) AND (CMAKE_GENERATOR STREQUAL Xcode) AND (NAME_HAS_DOT EQUAL -1) AND (NAME_HAS_TILDE GREATER -1))
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME '${PLUGIN_NAME}')
  else()
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PLUGIN_NAME})
  endif()

  # Generate the function to export for Windows
  if(${WIN32})
    set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS "/export:${EXPORT_FUNCTION}")
  endif()

  # Defines the output path of the plugin.
  set_target_properties(${PROJECT_NAME} PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${CSOUND_OUTPUT_PATH}
    RUNTIME_OUTPUT_DIRECTORY ${CSOUND_OUTPUT_PATH}
    ARCHIVE_OUTPUT_DIRECTORY ${CSOUND_OUTPUT_PATH})
    
  foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set_target_properties(${PROJECT_NAME} PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CSOUND_OUTPUT_PATH}
      RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CSOUND_OUTPUT_PATH}
      ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CSOUND_OUTPUT_PATH})
  endforeach(OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES)

  message(STATUS "*** setting install destination: ${PLUGIN_INSTALL_DIR}")
  
  install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION "${PLUGIN_INSTALL_DIR}")

endfunction(add_csound_plugin)

# The macro defines the output path of the plugins.
macro(set_csound_build_path PLUGIN_PATH)
  set(CSOUND_OUTPUT_PATH ${PLUGIN_PATH})
endmacro(set_csound_build_path)

# The macro sets the location of csound sources.
macro(set_csound_sources CSOUND_SOURCES)
  set(CSOUND_SOURCES_PATH ${CSOUND_SOURCES})
endmacro(set_csound_sources)

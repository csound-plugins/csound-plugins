option(BUILD_PATHTOOLS_OPCODES "Build pathtools opcodes" ON)

if(NOT(BUILD_PATHTOOLS_OPCODES))
  return()
endif()


make_c_plugin(pathtools src/pathtools.c)


if(WIN32XX)
  # sndfile should be installed via vcpkg
  message(STATUS ">>>>>>>>>>>>>>> Building using msvc")
  find_package(SndFile REQUIRED)
  target_link_libraries(pathtools PRIVATE SndFile::sndfile)
else()
  find_path(LIBSNDFILE_INCLUDE_DIRS sndfile.h)
  find_library(LIBSNDFILE_LIBRARIES sndfile)
  message(STATUS "libsndfile libs: ${LIBSNDFILE_LIBRARIES}")
  target_include_directories(pathtools PRIVATE "${LIBSNDFILE_INCLUDE_DIRS}")
  target_link_libraries(pathtools PRIVATE "${LIBSNDFILE_LIBRARIES}")      
endif()
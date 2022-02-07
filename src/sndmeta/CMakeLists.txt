option(BUILD_SNDMETA_OPCODES "Build sndmeta opcodes" ON)

if(NOT(BUILD_SNDMETA_OPCODES))
  return()
endif()


make_c_plugin(sndmeta src/sndmeta.c)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


if(WIN32)
  # sndfile should be installed via vcpkg
  message(STATUS ">>>>>>>>>>>>>>> Building using msvc")
  find_package(SndFile REQUIRED)
  target_link_libraries(sndmeta PRIVATE SndFile::sndfile)
else()
  find_path(LIBSNDFILE_INCLUDE_DIRS sndfile.h)
  find_library(LIBSNDFILE_LIBRARIES sndfile)
  message(STATUS "*** libsndfile libs: ${LIBSNDFILE_LIBRARIES}")
  target_include_directories(sndmeta PRIVATE "${LIBSNDFILE_INCLUDE_DIRS}")
  target_link_libraries(sndmeta PRIVATE "${LIBSNDFILE_LIBRARIES}") 
endif()

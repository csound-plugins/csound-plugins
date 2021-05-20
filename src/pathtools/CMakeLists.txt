option(BUILD_PATHTOOLS_OPCODES "Build pathtools opcodes" ON)

if(NOT(BUILD_PATHTOOLS_OPCODES))
  return()
endif()

find_path(LIBSNDFILE_INCLUDE_DIRS sndfile.h)
find_library(LIBSNDFILE_LIBRARIES sndfile)

make_c_plugin(pathtools src/pathtools.c)
target_include_directories(pathtools PRIVATE "${LIBSNDFILE_INCLUDE_DIRS}")
message(STATUS "libsndfile libs: ${LIBSNDFILE_LIBRARIES}")
target_link_libraries(pathtools PRIVATE "${LIBSNDFILE_LIBRARIES}")
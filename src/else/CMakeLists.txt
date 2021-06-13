option(BUILD_ELSE_OPCODES "Build else opcodes" ON)

if(NOT(BUILD_ELSE_OPCODES))
  return()
endif()

find_package(FLUIDSYNTH)

make_plugin(else src/else.c)
target_include_directories(else PRIVATE ${FLUIDSYNTH_INCLUDE_DIR})
target_link_libraries(else ${FLUIDSYNTH_LIBRARIES})


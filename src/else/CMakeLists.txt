option(BUILD_ELSE_OPCODES "Build else opcodes" ON)

if(NOT(BUILD_ELSE_OPCODES))
  return()
endif()

make_plugin(else src/else.c)
target_link_libraries(else fluidsynth)

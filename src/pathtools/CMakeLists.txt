option(BUILD_PATHTOOLS_OPCODES "Build pathtools opcodes" ON)

if(NOT(BUILD_PATHTOOLS_OPCODES))
  return()
endif()

make_plugin(pathtools src/pathtools.c)

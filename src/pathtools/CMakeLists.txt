option(BUILD_PATHTOOLS_OPCODES "Build pathtools opcodes" ON)

if(BUILD_PATHTOOLS_OPCODES)
  make_plugin(pathtools src/pathtools.c)
endif()

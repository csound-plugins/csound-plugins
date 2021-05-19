option(BUILD_RORY_OPCODES "Build Rory opcodes" ON)

if(NOT(BUILD_RORY_OPCODES))
  return()
endif()

make_plugin(rory src/opcodes)

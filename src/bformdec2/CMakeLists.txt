option(BUILD_BFORMDEC2_OPCODES "Build bformenc2 opcodes" ON)

if(NOT(BUILD_BFORMDEC2_OPCODES))
  return()
endif()

make_plugin(bformdec2 bformdec2/bformdec2.cpp)

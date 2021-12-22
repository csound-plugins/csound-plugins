option(BUILD_BEOSC_OPCODES "Build beosc opcodes" ON)

if(BUILD_BEOSC_OPCODES)
  make_plugin(beosc beosc.c)
endif()

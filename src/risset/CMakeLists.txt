option(BUILD_RISSET_OPCODES "Build risset opcodes" ON)

if(BUILD_RISSET_OPCODES)
  make_plugin(risset src/risset.c)
endif()

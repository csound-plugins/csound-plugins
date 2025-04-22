option(BUILD_TESTPLUGIN_OPCODES "Build test plugin" ON)

if(BUILD_TESTPLUGIN_OPCODES)
  make_plugin(testplugin src/testplugin.c)
endif()

option(BUILD_TESTPLUGIN_OPCODES "Build test plugin" OFF)

if(BUILD_TESTPLUGIN_OPCODES)
  make_plugin(testplugin src/testplugin.c)
endif()

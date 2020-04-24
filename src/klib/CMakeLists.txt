option(BUILD_KLIB_OPCODES "Build klib opcodes" ON)

if(BUILD_KLIB_OPCODES)
  make_plugin(klib src/klib.c)
endif()

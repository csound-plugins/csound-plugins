option(BUILD_POLY_OPCODES "Build poly opcodes" ON)

if(BUILD_POLY_OPCODES)
  make_plugin(poly src/poly.c)
endif()

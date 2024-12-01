option(BUILD_POODLE_OPCODES "Build poodle opcodes" ON)

if(BUILD_POODLE_OPCODES)
  make_plugin(poodle src/poodle.cpp)
endif()

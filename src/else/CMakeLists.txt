option(BUILD_ELSE_OPCODES "Build else opcodes" ON)

if(NOT(BUILD_ELSE_OPCODES))
  return()
endif()

# find_package(FLUIDSYNTH)

make_plugin(else src/else.c)

include(CheckLibraryExists)

CHECK_LIBRARY_EXISTS(m sin "" HAVE_LIB_M)                                                                                                
                                                                                                                                         
if (HAVE_LIB_M)                                                                                                                          
  set(EXTRA_LIBS ${EXTRA_LIBS} m)
  MESSAGE("else: Adding libm")                                                                                                    
endif (HAVE_LIB_M)

target_link_libraries(else PUBLIC ${EXTRA_LIBS})
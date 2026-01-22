file(GLOB_RECURSE BSMP_SRC *.cpp)
add_executable(bsmp ${BSMP_SRC})
target_link_libraries(bsmp bytecode type)
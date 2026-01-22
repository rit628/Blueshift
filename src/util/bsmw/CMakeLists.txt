file(GLOB_RECURSE BSMW_SRC *.cpp)
add_executable(bsmw ${BSMW_SRC})
target_link_libraries(bsmw bytecode type)
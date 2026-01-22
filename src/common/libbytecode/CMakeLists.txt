file(GLOB_RECURSE LIBBYTECODE_SRC *.cpp)
add_library(bytecode STATIC ${LIBBYTECODE_SRC})
target_include_directories(bytecode PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(bytecode network)
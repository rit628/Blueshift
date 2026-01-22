file(GLOB_RECURSE LIBBYTECODE_SRC *.cpp)
add_library(bytecode STATIC ${LIBBYTECODE_SRC})
target_include_directories(bytecode PUBLIC .)
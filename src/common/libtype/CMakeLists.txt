file(GLOB_RECURSE LIBTYPE_SRC *.cpp)
add_library(type STATIC ${LIBTYPE_SRC})
target_include_directories(type PUBLIC .)
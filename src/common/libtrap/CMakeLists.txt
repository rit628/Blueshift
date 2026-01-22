file(GLOB_RECURSE LIBTRAP_SRC *.cpp)
add_library(trap STATIC ${LIBTRAP_SRC})
target_include_directories(trap PUBLIC .)
target_link_libraries(trap EM type lang_common)
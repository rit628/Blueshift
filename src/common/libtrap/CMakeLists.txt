file(GLOB_RECURSE LIBTRAP_SRC *.cpp)
add_library(trap STATIC ${LIBTRAP_SRC})
target_include_directories(trap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(trap EM type bytecode)
file(GLOB_RECURSE LIBNETWORK_SRC *.cpp)
add_library(network STATIC ${LIBNETWORK_SRC})
target_include_directories(network PUBLIC .)
target_link_libraries(network TSQ)
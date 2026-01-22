file(GLOB_RECURSE LIBCLIENTROUTER_SRC *.cpp)
add_library(client_router STATIC ${LIBCLIENTROUTER_SRC})
target_include_directories(client_router PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client_router network)
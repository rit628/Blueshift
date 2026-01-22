file(GLOB_RECURSE LIBCLIENTGATEWAY_SRC *.cpp)
add_library(client_gateway STATIC ${LIBCLIENTGATEWAY_SRC})
target_include_directories(client_gateway PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client_gateway scheduler processing network virtual_machine)
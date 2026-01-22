file(GLOB_RECURSE LIBCE_SRC *.cpp)
add_library(client_engine STATIC ${LIBCE_SRC})
target_include_directories(client_engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(client_engine network device client_gateway)
file(GLOB_RECURSE LIBDEPGRAPH_SRC *.cpp)
add_library(depgraph STATIC ${LIBDEPGRAPH_SRC})
target_include_directories(depgraph PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(depgraph lang_common type)
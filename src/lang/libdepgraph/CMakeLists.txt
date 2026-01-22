file(GLOB_RECURSE LIBDEPGRAPH_SRC *.cpp)
add_library(depgraph STATIC ${LIBDEPGRAPH_SRC})
target_include_directories(depgraph PUBLIC .)
target_link_libraries(depgraph lang_common type)
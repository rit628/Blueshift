file(GLOB_RECURSE LIBSYMGRAPH_SRC *.cpp)
add_library(symgraph STATIC ${LIBSYMGRAPH_SRC})
target_include_directories(symgraph PUBLIC .)
target_link_libraries(symgraph depgraph lang_common type)
file(GLOB_RECURSE LIBCOMPILER_SRC *.cpp)
add_library(compiler STATIC ${LIBCOMPILER_SRC})
target_include_directories(compiler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(compiler lang_common lexer parser interpreter analyzer generator depgraph symgraph divider)
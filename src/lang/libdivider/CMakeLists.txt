file(GLOB_RECURSE LIBDIVIDER_SRC *.cpp)
add_library(divider STATIC ${LIBDIVIDER_SRC})
target_include_directories(divider PUBLIC .)
target_link_libraries(divider symgraph depgraph lang_common type)
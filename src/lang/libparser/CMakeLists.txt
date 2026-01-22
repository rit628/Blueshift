file(GLOB_RECURSE LIBPARSER_SRC *.cpp)
add_library(parser STATIC ${LIBPARSER_SRC})
target_include_directories(parser PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parser lang_common)
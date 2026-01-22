file(GLOB_RECURSE LIBLEXER_SRC *.cpp)
add_library(lexer STATIC ${LIBLEXER_SRC})
target_include_directories(lexer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lexer lang_common)
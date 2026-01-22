file(GLOB_RECURSE LIBANALYZER_SRC *.cpp)
add_library(analyzer STATIC ${LIBANALYZER_SRC})
target_include_directories(analyzer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(analyzer lang_common type trap)
file(GLOB_RECURSE LANG_COMMON_SRC *.cpp)
add_library(lang_common STATIC ${LANG_COMMON_SRC})
target_include_directories(lang_common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(lang_common type network)
file(GLOB_RECURSE LIBGENERATOR_SRC *.cpp)
add_library(generator STATIC ${LIBGENERATOR_SRC})
target_include_directories(generator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(generator lang_common type trap)
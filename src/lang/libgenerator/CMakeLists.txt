file(GLOB_RECURSE LIBGENERATOR_SRC *.cpp)
add_library(generator STATIC ${LIBGENERATOR_SRC})
target_include_directories(generator PUBLIC .)
target_link_libraries(generator lang_common type trap)
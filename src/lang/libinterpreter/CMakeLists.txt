file(GLOB_RECURSE LIBINTERPRETER_SRC *.cpp)
add_library(interpreter STATIC ${LIBINTERPRETER_SRC})
target_include_directories(interpreter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(interpreter lang_common type trap)
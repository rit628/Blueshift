file(GLOB_RECURSE LIBVIRTUALMACHINE_SRC *.cpp)
add_library(virtual_machine STATIC ${LIBVIRTUALMACHINE_SRC})
target_include_directories(virtual_machine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(virtual_machine lang_common type bytecode trap)
file(GLOB_RECURSE LIBVIRTUALMACHINE_SRC *.cpp)
add_library(virtual_machine STATIC ${LIBVIRTUALMACHINE_SRC})
target_include_directories(virtual_machine PUBLIC .)
target_link_libraries(virtual_machine lang_common type bytecode trap)
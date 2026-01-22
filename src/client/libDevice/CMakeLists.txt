file(GLOB_RECURSE LIBDEVICE_SRC *.cpp)
add_library(device STATIC ${LIBDEVICE_SRC} $<TARGET_OBJECTS:drivers>)
target_include_directories(device PUBLIC .)
target_link_libraries(device network dynamic_message)
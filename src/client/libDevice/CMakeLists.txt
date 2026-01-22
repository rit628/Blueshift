file(GLOB_RECURSE LIBDEVICE_SRC *.cpp)
add_library(device STATIC ${LIBDEVICE_SRC} $<TARGET_OBJECTS:drivers>)
target_include_directories(device PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(device network dynamic_message TSM)

add_library(devtypes INTERFACE)
target_include_directories(devtypes INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} devtypes)
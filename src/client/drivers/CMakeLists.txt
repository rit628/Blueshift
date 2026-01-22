file(GLOB_RECURSE DRIVER_SRC *.cpp)
add_library(drivers OBJECT ${DRIVER_SRC}) # must be an object library to ensure strong symbols for controller dependent drivers are utilized
target_link_libraries(drivers devtypes dynamic_message network)
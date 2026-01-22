file(GLOB_RECURSE LIBEM_SRC *.cpp)
add_library(EM STATIC ${LIBEM_SRC})
target_include_directories(EM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(EM scheduler TSM TSQ MM dynamic_message interpreter virtual_machine)
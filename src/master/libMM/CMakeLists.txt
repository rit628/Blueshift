file(GLOB_RECURSE LIBMM_SRC *.cpp)
add_library(MM STATIC ${LIBMM_SRC})
target_include_directories(MM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MM network TSQ dynamic_message)
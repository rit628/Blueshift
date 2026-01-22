file(GLOB_RECURSE LIBNM_SRC *.cpp)
add_library(NM STATIC ${LIBNM_SRC})
target_include_directories(NM PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(NM network dynamic_message ticker)
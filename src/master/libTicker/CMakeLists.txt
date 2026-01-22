file(GLOB_RECURSE LIBTICKER_SRC *.cpp)
add_library(ticker STATIC ${LIBTICKER_SRC})
target_include_directories(ticker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(ticker network)
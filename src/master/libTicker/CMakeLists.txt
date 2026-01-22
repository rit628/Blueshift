file(GLOB_RECURSE LIBTICKER_SRC *.cpp)
add_library(ticker STATIC ${LIBTICKER_SRC})
target_include_directories(ticker PUBLIC .)
target_link_libraries(ticker network TSQ)
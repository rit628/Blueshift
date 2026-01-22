file(GLOB_RECURSE LIBSCHEDULER_SRC *.cpp)
add_library(scheduler STATIC ${LIBSCHEDULER_SRC})
target_include_directories(scheduler PUBLIC .)
target_link_libraries(scheduler TSQ network)
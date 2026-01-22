file(GLOB_RECURSE LIBSCHEDULER_SRC *.cpp)
add_library(scheduler STATIC ${LIBSCHEDULER_SRC})
target_include_directories(scheduler PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(scheduler TSQ network)
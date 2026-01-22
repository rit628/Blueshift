add_subdirectory(libTicker)
add_subdirectory(libEM)
add_subdirectory(libMM)
add_subdirectory(libNM)

add_executable(master main.cpp)
target_link_libraries(master TSQ EM MM NM compiler)
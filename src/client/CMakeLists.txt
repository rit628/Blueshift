add_subdirectory(drivers)
add_subdirectory(libDevice)
add_subdirectory(libClientGateway)
add_subdirectory(libCE)

add_executable(client main.cpp)
target_link_libraries(client "$<LINK_LIBRARY:WHOLE_ARCHIVE,device>" client_engine) # whole library archive is linked to check for strong symbols

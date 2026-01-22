add_subdirectory(drivers)
add_subdirectory(libDevice)
add_subdirectory(libClientGateway)
add_subdirectory(libCE)

add_executable(client main.cpp)
# whole library archive is included for libDevice to allow strong symbols
# from controller dependent drivers to overwrite default implementations (weak symbols)
target_link_libraries(client "$<LINK_LIBRARY:WHOLE_ARCHIVE,device>" client_engine)

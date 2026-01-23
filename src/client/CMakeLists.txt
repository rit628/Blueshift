add_subdirectory(drivers)
add_subdirectory(libDevice)
add_subdirectory(libClientGateway)
add_subdirectory(libClientRouter)
add_subdirectory(libCE)

# whole library archive is included for libDevice to allow strong symbols
# from controller dependent drivers to overwrite default implementations (weak symbols)
bls_add_executable(client main.cpp LINKS "$<LINK_LIBRARY:WHOLE_ARCHIVE,device>" client_engine)
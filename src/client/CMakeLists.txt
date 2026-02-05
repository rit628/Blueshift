add_subdirectory(drivers)
add_subdirectory(libDevice)
add_subdirectory(libClientGateway)
add_subdirectory(libClientRouter)
add_subdirectory(libCE)

bls_add_executable(client main.cpp LINKS device client_engine)
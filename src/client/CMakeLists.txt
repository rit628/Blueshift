add_subdirectory(drivers)
add_subdirectory(libDevice)
add_subdirectory(libClientGateway)
add_subdirectory(libClientRouter)
add_subdirectory(libCE)

bls_add_executable(client main.cpp LINKS device client_engine)
if(EMSCRIPTEN)
    target_link_options(client PRIVATE -sPROXY_TO_PTHREAD=1) # ensure main thread does not get blocked
endif()
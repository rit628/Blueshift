add_subdirectory(libTicker)
add_subdirectory(libEM)
add_subdirectory(libMM)
add_subdirectory(libNM)

bls_add_executable(master main.cpp LINKS TSQ EM MM NM compiler)
add_subdirectory(libTicker)
add_subdirectory(libEM)
add_subdirectory(libMM)
add_subdirectory(libNM)

bls_add_executable(master main.cpp LINKS TSQ EM MM NM compiler)
if(EMSCRIPTEN)
    target_link_options(master PRIVATE
                        -sPROXY_TO_PTHREAD=1 # ensure main thread does not get blocked
                        -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency # set thread count to core count
                      )
endif()
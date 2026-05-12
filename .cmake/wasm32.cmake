set(EMSCRIPTEN_FORCE_COMPILERS OFF)
include(/wasm32/cmake/Modules/Platform/Emscripten.cmake) # use emscripten toolchain as base

# COMPILER CONFIG
add_compile_options(
                        -pthread # threads
                        -fwasm-exceptions # exceptions
                    )
add_link_options(
                    -pthread # threads
                    -fwasm-exceptions # exceptions
                    -sUSE_BOOST_HEADERS=1 # use boost ports
                )

# LINKER CONFIG
set(CMAKE_LINKER_TYPE DEFAULT) # use wasm-ld

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(__WASM32__)

# DISABLED LIBS
set(USE_SYSTEM_BOOST_HEADERS true)
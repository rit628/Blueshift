set(EMSCRIPTEN_FORCE_COMPILERS OFF)
include(/wasm32/cmake/Modules/Platform/Emscripten.cmake) # use emscripten toolchain as base

# COMPILER CONFIG
add_compile_options(-pthread -fwasm-exceptions) # enable threads and exception support
add_link_options(-fwasm-exceptions -sNODERAWFS=1) # exceptions and filesystem

# LINKER CONFIG
set(CMAKE_LINKER_TYPE DEFAULT) # use wasm-ld

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="WEB")
add_compile_definitions(__WASM32__)

# temporary hack to get asio to build; probably breaks at runtime
set(BOOST_CONTEXT_IMPLEMENTATION "ucontext" CACHE INTERNAL "Boost.Context implementation (fcontext, ucontext, winfib)" FORCE)

# DISABLED LIBS
set(DISABLE_CURL true)
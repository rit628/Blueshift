set(EMSCRIPTEN_FORCE_COMPILERS OFF)
include(/wasm32/cmake/Modules/Platform/Emscripten.cmake) # use emscripten toolchain as base

# COMPILER CONFIG
add_compile_options(-pthread) # enable threads

# LINKER CONFIG
set(CMAKE_LINKER_TYPE DEFAULT) # use wasm-ld

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="WEB")

# temporary hack to get asio to build; probably breaks at runtime
set(BOOST_CONTEXT_IMPLEMENTATION "ucontext" CACHE INTERNAL "Boost.Context implementation (fcontext, ucontext, winfib)" FORCE)

# DISABLED LIBS
set(DISABLE_CURL true)
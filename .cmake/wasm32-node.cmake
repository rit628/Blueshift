include(${CMAKE_CURRENT_LIST_DIR}/wasm32.cmake) # use wasm32 toolchain as base

# COMPILER CONFIG
set(CMAKE_EXECUTABLE_SUFFIX_C ".js")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".js")
add_link_options(-sNODERAWFS=1) # filesystem support

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="NODEJS")

# DISABLED LIBS
set(DISABLE_SDL true) # SDL doesnt work with node for some reason
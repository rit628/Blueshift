include(${CMAKE_CURRENT_LIST_DIR}/wasm32.cmake) # use wasm32 toolchain as base

# COMPILER CONFIG
add_link_options(-sNODERAWFS=1) # filesystem support

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="NODEJS")
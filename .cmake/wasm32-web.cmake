include(${CMAKE_CURRENT_LIST_DIR}/wasm32.cmake) # use wasm32 toolchain as base
set(CMAKE_EXECUTABLE_SUFFIX_C ".html")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".html")

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="WEB")
include(${CMAKE_CURRENT_LIST_DIR}/wasm32.cmake) # use wasm32 toolchain as base
set(CMAKE_EXECUTABLE_SUFFIX_C ".html")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".html")

# COMPILER CONFIG
add_link_options(
                    -lidbfs.js # LDBFS for persistent storage in the browser
                    -sEXPORT_ES6=1 # Create ES6 modules
                    -sEXIT_RUNTIME=1 # Runtimes will exit on end of main()
                    -sFORCE_FILESYSTEM # Filesystem support
                    -sEXPORTED_RUNTIME_METHODS=['FS','IDBFS'] # Filesystem objects
                )

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="WEB")
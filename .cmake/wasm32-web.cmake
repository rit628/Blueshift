include(${CMAKE_CURRENT_LIST_DIR}/wasm32.cmake) # use wasm32 toolchain as base

# COMPILER CONFIG
set(CMAKE_EXECUTABLE_SUFFIX_C ".html")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".html")
add_link_options(
                    -sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency # set thread count to core count
                    -sPTHREAD_POOL_SIZE_STRICT=0 # allow thread pool exhaustion

                    -sEXPORT_ES6=1 # Create ES6 modules

                    -sEXIT_RUNTIME=1 # Runtimes will exit on end of main()
                    
                    -sFORCE_FILESYSTEM # Filesystem support
                    -lidbfs.js # LDBFS for persistent storage in the browser
                    -sEXPORTED_RUNTIME_METHODS=['FS','IDBFS'] # Filesystem objects
                )

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="WEB")
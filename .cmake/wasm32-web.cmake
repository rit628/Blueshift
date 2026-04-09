include(${CMAKE_CURRENT_LIST_DIR}/wasm32.cmake) # use wasm32 toolchain as base

# COMPILER CONFIG
set(CMAKE_EXECUTABLE_SUFFIX_C ".html")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".html")
add_link_options(
                    -sPTHREAD_POOL_SIZE_STRICT=0 # allow thread pool exhaustion

                    -lwebsocket.js # access to emscripten websocket api for networking

                    -sEXPORT_ES6=1 # create ES6 modules

                    -sEXIT_RUNTIME=1 # runtimes will exit on end of main()
                    
                    -sFORCE_FILESYSTEM # filesystem support
                    -lidbfs.js # LDBFS for persistent storage in the browser
                    -sEXPORTED_RUNTIME_METHODS=['FS','IDBFS'] # filesystem objects
                )

# CONDITIONAL COMPILATION VARIABLES
add_compile_definitions(CONTROLLER_TARGET="WEB")

# DISABLED LIBS
set(DISABLE_SDL true) # SDL3 currently broken on web
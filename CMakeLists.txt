cmake_minimum_required(VERSION 3.29) # for some reason setting 3.30 causes redundant rebuilds

project($ENV{PROJECT_NAME})

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SCAN_FOR_MODULES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# Helper functions
include(.cmake/macros.cmake)

# Packages
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# GoogleTest
FetchContent_Declare(
    googletest
    SYSTEM # Disable warnings in headers
    URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Boost
set(REQUIRED_BOOST_LIBS asio thread regex lockfree type_index math json serialization beast url)
list(TRANSFORM REQUIRED_BOOST_LIBS PREPEND Boost:: OUTPUT_VARIABLE BOOST_LINK_LIBRARIES)
FetchContent_Declare(
    Boost
    SYSTEM # Disable warnings in headers
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.zip
)
set(BOOST_INCLUDE_LIBRARIES ${REQUIRED_BOOST_LIBS})

FetchContent_Declare(
    curl
    SYSTEM # Disable warnings in headers
    URL https://github.com/curl/curl/releases/download/curl-8_15_0/curl-8.15.0.zip
)

# SDL3
FetchContent_Declare(
    SDL3
    SYSTEM # Disable warnings in headers
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.2.16.zip
)

if(NOT DISABLE_SDL)
    add_compile_definitions(SDL_ENABLED)
    set(FETCHCONTENT_OPTIONAL ${FETCHCONTENT_OPTIONAL} SDL3)
    set(SYSTEM_LINK_LIBRARIES ${SYSTEM_LINK_LIBRARIES} SDL3::SDL3)
endif()

if(NOT DISABLE_CURL)
    set(FETCHCONTENT_OPTIONAL ${FETCHCONTENT_OPTIONAL} curl)
    set(SYSTEM_LINK_LIBRARIES ${SYSTEM_LINK_LIBRARIES} ssl crypto)
endif()

set(CMAKE_CXX_FLAGS "-w") # Disable warnings for fetched content
FetchContent_MakeAvailable(googletest Boost ${FETCHCONTENT_OPTIONAL})

# Set compile options after fetch content to suppress external warnings
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wold-style-cast -Wno-sign-compare")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -glldb -gfull")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Og -glldb -DNDEBUG")
    
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{RUNTIME_OUTPUT_DIRECTORY})

include_directories(${curl_SOURCE_DIR}/include)
link_libraries(${BOOST_LINK_LIBRARIES} ${SYSTEM_LINK_LIBRARIES})

# Preprocessor Macro Definitions
if(NOT CMAKE_CROSSCOMPILING)
    string(TOUPPER ${CMAKE_SYSTEM_NAME} CONTROLLER_TARGET)
    add_compile_definitions(CONTROLLER_TARGET="${CONTROLLER_TARGET}")
endif()
add_compile_definitions(BROADCAST_PORT=$ENV{BROADCAST_PORT})
add_compile_definitions(MASTER_PORT=$ENV{MASTER_PORT})

enable_testing()

include_directories(${CMAKE_SOURCE_DIR}/config)
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/test)
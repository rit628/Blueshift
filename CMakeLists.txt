cmake_minimum_required(VERSION 3.29) # for some reason setting 3.30 causes redundant rebuilds

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_LINKER_TYPE LLD)
set(CMAKE_AR llvm-ar)

project($ENV{PROJECT_NAME})

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SCAN_FOR_MODULES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
SET(CMAKE_BUILD_RPATH_USE_ORIGIN TRUE)

# Helper definitions
set(SCRIPT_DIR ${CMAKE_SOURCE_DIR}/scripts)
set(GEN_DIR ${CMAKE_SOURCE_DIR}/$ENV{GENERATED_OUTPUT_DIRECTORY})

# Helper functions
include(.cmake/macros.cmake)

# Packages
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_BASE_DIR ${CMAKE_SOURCE_DIR}/$ENV{DEPENDENCY_DIRECTORY})

# GoogleTest
FetchContent_Declare(
    googletest
    SYSTEM # Disable warnings in headers
    URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Boost
set(REQUIRED_BOOST_HEADER_LIBRARIES asio lockfree type_index beast)
set(REQUIRED_BOOST_LINK_LIBRARIES regex math json serialization url)
FetchContent_Declare(
    Boost
    SYSTEM # Disable warnings in headers
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.zip
)
set(BOOST_INCLUDE_LIBRARIES ${REQUIRED_BOOST_LINK_LIBRARIES})
list(TRANSFORM REQUIRED_BOOST_LINK_LIBRARIES PREPEND Boost:: OUTPUT_VARIABLE BOOST_LINK_LIBRARIES)
if(NOT USE_SYSTEM_BOOST_HEADERS)
    set(BOOST_INCLUDE_LIBRARIES ${REQUIRED_BOOST_HEADER_LIBRARIES} ${BOOST_INCLUDE_LIBRARIES})
    list(TRANSFORM REQUIRED_BOOST_HEADER_LIBRARIES PREPEND Boost:: OUTPUT_VARIABLE BOOST_HEADER_LIBRARIES)
endif()

# Flatbuffers
FetchContent_Declare(
    flatbuffers
    SYSTEM # Disable warnings in headers
    URL https://github.com/google/flatbuffers/archive/refs/tags/v$ENV{FLATBUFFERS_VERSION}.zip
)
set(FLATBUFFERS_BUILD_FLATC OFF)
set(FLATBUFFERS_BUILD_TESTS OFF)
set(FLATBUFFERS_FLATC_EXECUTABLE flatc)

# SDL3
FetchContent_Declare(
    SDL3
    SYSTEM # Disable warnings in headers
    URL https://github.com/libsdl-org/SDL/archive/refs/tags/release-3.2.16.zip
)

if(NOT CMAKE_CROSSCOMPILING)
    set(FETCHCONTENT_OPTIONAL ${FETCHCONTENT_OPTIONAL} googletest)
endif()

if(NOT DISABLE_SDL)
    add_compile_definitions(SDL_ENABLED)
    set(FETCHCONTENT_OPTIONAL ${FETCHCONTENT_OPTIONAL} SDL3)
    set(SYSTEM_LINK_LIBRARIES ${SYSTEM_LINK_LIBRARIES} SDL3::SDL3)
endif()

get_directory_property(current_compile_options COMPILE_OPTIONS)
add_compile_options(-w) # Disable warnings for fetched content
FetchContent_MakeAvailable(Boost flatbuffers ${FETCHCONTENT_OPTIONAL})
set_directory_properties(PROPERTIES COMPILE_OPTIONS "${current_compile_options}") # restore old compile options

set(ESSENTIAL_FLAGS -Wall -Wextra -Wnon-virtual-dtor)
set(STYLE_FLAGS -Wmissing-prototypes -Wextra-semi -Wextra-semi-stmt -Wredundant-parens -Wsuggest-destructor-override -Wold-style-cast)
set(NEGATED_FLAGS -Wno-sign-compare -Wno-nontrivial-memcall)
# For performance critical code (ie. network buffers or space constrained classes)
set(SUGGESTED_FLAGS -Wweak-vtables -Wpadded)

# Set compile options after fetch content to suppress external warnings
add_compile_options(${ESSENTIAL_FLAGS} ${STYLE_FLAGS} ${NEGATED_FLAGS})
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -glldb -gfull")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Og -glldb -DNDEBUG")
    
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{RUNTIME_OUTPUT_DIRECTORY})

link_libraries(
    Boost::headers ${BOOST_HEADER_LIBRARIES} ${BOOST_LINK_LIBRARIES}
    flatbuffers
    ${SYSTEM_LINK_LIBRARIES}
)

# Generate flatbuffer structs
execute_process(COMMAND python3 ${SCRIPT_DIR}/driver.py ${SCRIPT_DIR}/gen/fbs_gen.py)
file(GLOB_RECURSE FB_SCHEMAS ${GEN_DIR}/schemas/*.fbs)
build_flatbuffers(
    "${FB_SCHEMAS}"
    ""
    fb_schemas
    ""
    ${GEN_DIR}
    ""
    ""
)
# Ensure all flatbuffer schemas are built
add_custom_target(fb_headers ALL DEPENDS fb_schemas)

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
cmake_minimum_required(VERSION 3.24)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SCAN_FOR_MODULES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Packages
include(FetchContent)

# GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Boost
set(REQUIRED_BOOST_LIBS asio thread regex type_index)
list(TRANSFORM REQUIRED_BOOST_LIBS PREPEND Boost:: OUTPUT_VARIABLE BOOST_LIB_DEPS)
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.zip
)
set(BOOST_INCLUDE_LIBRARIES ${REQUIRED_BOOST_LIBS})
FetchContent_MakeAvailable(Boost)

project($ENV{PROJECT_NAME})
    
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{RUNTIME_OUTPUT_DIRECTORY})

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(MASTER_SRC_DIR ${SRC_DIR}/master)
set(CLIENT_SRC_DIR ${SRC_DIR}/client)
set(LANG_SRC_DIR ${SRC_DIR}/lang)

set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
set(TEST_DEPS ${TEST_DIR}/include)
set(MASTER_TEST_DIR ${TEST_DIR}/master)
set(CLIENT_TEST_DIR ${TEST_DIR}/client)
set(LANG_TEST_DIR ${TEST_DIR}/lang)

# MASTER LIBS AND EXECUTABLE
file(GLOB_RECURSE LIBTSQ_SRC ${MASTER_SRC_DIR}/libTSQ/*.cpp)
add_library(TSQ STATIC ${LIBTSQ_SRC})

file(GLOB_RECURSE LIBDM_SRC ${MASTER_SRC_DIR}/libDM/*.cpp)
add_library(dynamic_message STATIC ${LIBDM_SRC})
target_link_libraries(dynamic_message ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBVM_SRC ${MASTER_SRC_DIR}/libVM/*.cpp)
add_library(virtual_machine STATIC ${LIBVM_SRC})
target_link_libraries(virtual_machine dynamic_message)

file(GLOB_RECURSE LIBBYTEPROC_SRC ${MASTER_SRC_DIR}/libByteproc/*.cpp)
add_library(byteproc STATIC ${LIBBYTEPROC_SRC})
target_link_libraries(byteproc virtual_machine)

add_executable(master ${MASTER_SRC_DIR}/main.cpp)

# CLIENT LIBS AND EXECUTABLE
add_executable(client ${CLIENT_SRC_DIR}/main.cpp)

# LANG LIBS AND EXECUTABLE
file(GLOB_RECURSE LIBLEXER_SRC ${LANG_SRC_DIR}/liblexer/*.cpp)
add_library(lexer STATIC ${LIBLEXER_SRC})
target_link_libraries(lexer ${BOOST_LIB_DEPS})

add_executable(lang ${LANG_SRC_DIR}/main.cpp)
target_link_libraries(lang lexer ${BOOST_LIB_DEPS})

# TESTS
enable_testing()

# LANG TESTS
file(GLOB_RECURSE LIBLEXER_TESTS ${LANG_TEST_DIR}/liblexer/*.cpp)
add_executable(test_liblexer ${LIBLEXER_TESTS})
target_include_directories(test_liblexer PUBLIC ${TEST_DEPS} ${LANG_SRC_DIR}/liblexer)
target_link_libraries(test_liblexer lexer GTest::gtest_main)

# MASTER TESTS
file(GLOB_RECURSE LIBTSQ_TESTS ${MASTER_TEST_DIR}/libTSQ/*.cpp)
add_executable(test_libTSQ ${LIBTSQ_TESTS})
target_include_directories(test_libTSQ PUBLIC ${TEST_DEPS} ${MASTER_SRC_DIR}/libTSQ)
target_link_libraries(test_libTSQ TSQ GTest::gtest_main)

file(GLOB_RECURSE LIBDM_TESTS ${MASTER_TEST_DIR}/libDM/*.cpp)
add_executable(test_libDM ${LIBDM_TESTS})
target_include_directories(test_libDM PUBLIC ${TEST_DEPS} ${MASTER_SRC_DIR}/libDM)
target_link_libraries(test_libDM dynamic_message GTest::gtest_main)

file(GLOB_RECURSE LIBVM_TESTS ${MASTER_TEST_DIR}/libVM/*.cpp)
add_executable(test_libVM ${LIBVM_TESTS})
target_include_directories(test_libVM PUBLIC ${TEST_DEPS} ${MASTER_SRC_DIR}/libByteproc)
target_link_libraries(test_libVM byteproc GTest::gtest_main)


include(GoogleTest)
gtest_discover_tests(test_liblexer)
gtest_discover_tests(test_libTSQ)
gtest_discover_tests(test_libDM)
gtest_discover_tests(
    test_libVM
    WORKING_DIRECTORY ${MASTER_TEST_DIR}/libVM
)

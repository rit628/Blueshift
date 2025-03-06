cmake_minimum_required(VERSION 3.25)

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
set(REQUIRED_BOOST_LIBS asio thread regex lockfree type_index math)
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

set(BLS_CONFIG ${CMAKE_SOURCE_DIR}/config)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(COMMON_SRC_DIR ${SRC_DIR}/common)
set(MASTER_SRC_DIR ${SRC_DIR}/master)
set(CLIENT_SRC_DIR ${SRC_DIR}/client)
set(LANG_SRC_DIR ${SRC_DIR}/lang)

set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
set(TEST_DEPS ${TEST_DIR}/include)
set(COMMON_TEST_DIR ${TEST_DIR}/common)
set(MASTER_TEST_DIR ${TEST_DIR}/master)
set(CLIENT_TEST_DIR ${TEST_DIR}/client)
set(LANG_TEST_DIR ${TEST_DIR}/lang)

set(LANG_COMMON ${LANG_SRC_DIR}/common)

# COMMON LIBS
file(GLOB_RECURSE LIBTSQ_SRC ${COMMON_SRC_DIR}/libTSQ/*.cpp)
add_library(TSQ STATIC ${LIBTSQ_SRC})

file(GLOB_RECURSE LIBHD_SRC ${COMMON_SRC_DIR}/libHD/*.cpp)
add_library(heap_descriptor STATIC ${LIBHD_SRC})
target_include_directories(heap_descriptor PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})

file(GLOB_RECURSE LIBDM_SRC ${COMMON_SRC_DIR}/libDM/*.cpp)
add_library(dynamic_message STATIC ${LIBDM_SRC})
target_include_directories(dynamic_message PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(dynamic_message heap_descriptor ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBNETWORK_SRC ${COMMON_SRC_DIR}/libnetwork/*.cpp)
add_library(network STATIC ${LIBNETWORK_SRC})
target_include_directories(network PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(network TSQ ${BOOST_LIB_DEPS})

# MASTER LIBS AND EXECUTABLE
file(GLOB_RECURSE LIBTSM_SRC ${MASTER_SRC_DIR}/libTSM/*.cpp)
add_library(TSM STATIC ${LIBTSM_SRC})

file(GLOB_RECURSE LIBTICKER_SRC ${MASTER_SRC_DIR}/libTicker/*.cpp)
add_library(ticker STATIC ${LIBTICKER_SRC})
target_include_directories(ticker PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(ticker network TSQ ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBEM_SRC ${MASTER_SRC_DIR}/libEM/*.cpp)
add_library(EM STATIC ${LIBEM_SRC})
target_include_directories(EM PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${MASTER_SRC_DIR})
target_link_libraries(EM TSM TSQ dynamic_message ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBMM_SRC ${MASTER_SRC_DIR}/libMM/*.cpp)
add_library(MM STATIC ${LIBMM_SRC})
target_include_directories(MM PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${MASTER_SRC_DIR})
target_link_libraries(MM EM TSQ dynamic_message ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBNM_SRC ${MASTER_SRC_DIR}/libNM/*.cpp)
add_library(NM STATIC ${LIBNM_SRC})
target_include_directories(NM PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${MASTER_SRC_DIR})
target_link_libraries(NM network dynamic_message ticker ${BOOST_LIB_DEPS})

add_executable(master ${MASTER_SRC_DIR}/main.cpp)
target_link_libraries(master TSQ EM MM ${BOOST_LIB_DEPS})

# CLIENT LIBS AND EXECUTABLE
file(GLOB_RECURSE LIBDEVICE_SRC ${CLIENT_SRC_DIR}/libDevice/*.cpp)
add_library(device STATIC ${LIBDEVICE_SRC})
target_include_directories(device PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(device network dynamic_message ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBCE_SRC ${CLIENT_SRC_DIR}/libCE/*.cpp)
add_library(client_engine STATIC ${LIBCE_SRC})
target_include_directories(client_engine PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${CLIENT_SRC_DIR})
target_link_libraries(client_engine network device ${BOOST_LIB_DEPS})

add_executable(client ${CLIENT_SRC_DIR}/main.cpp)

# LANG LIBS AND EXECUTABLE
file(GLOB_RECURSE LANG_COMMON_SRC ${LANG_COMMON}/*.cpp)
add_library(lang_common STATIC ${LANG_COMMON_SRC})
target_include_directories(lang_common PUBLIC ${BLS_CONFIG})
target_link_libraries(lang_common ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBLEXER_SRC ${LANG_SRC_DIR}/liblexer/*.cpp)
add_library(lexer STATIC ${LIBLEXER_SRC})
target_include_directories(lexer PUBLIC ${BLS_CONFIG} ${LANG_COMMON})
target_link_libraries(lexer lang_common ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBPARSER_SRC ${LANG_SRC_DIR}/libparser/*.cpp)
add_library(parser STATIC ${LIBPARSER_SRC})
target_include_directories(parser PUBLIC ${BLS_CONFIG} ${LANG_COMMON})
target_link_libraries(parser lang_common ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBINTERPRETER_SRC ${LANG_SRC_DIR}/libinterpreter/*.cpp)
add_library(interpreter STATIC ${LIBINTERPRETER_SRC})
target_include_directories(interpreter PUBLIC ${BLS_CONFIG} ${LANG_COMMON})
target_link_libraries(interpreter lang_common ${BOOST_LIB_DEPS})

add_executable(lang ${LANG_SRC_DIR}/main.cpp)
target_link_libraries(lang lexer parser interpreter ${BOOST_LIB_DEPS})

# TESTS
enable_testing()

# COMMON TESTS
file(GLOB_RECURSE LIBTSQ_TESTS ${COMMON_TEST_DIR}/libTSQ/*.cpp)
add_executable(test_libTSQ ${LIBTSQ_TESTS})
target_include_directories(test_libTSQ PUBLIC ${TEST_DEPS} ${COMMON_SRC_DIR}/libTSQ)
target_link_libraries(test_libTSQ TSQ GTest::gtest_main)

file(GLOB_RECURSE LIBDM_TESTS ${COMMON_TEST_DIR}/libDM/*.cpp)
add_executable(test_libDM ${LIBDM_TESTS})
target_include_directories(test_libDM PUBLIC ${TEST_DEPS} ${COMMON_SRC_DIR}/libDM)
target_link_libraries(test_libDM dynamic_message GTest::gtest_main)

# LANG TESTS
file(GLOB_RECURSE LIBLEXER_TESTS ${LANG_TEST_DIR}/liblexer/*.cpp)
add_executable(test_liblexer ${LIBLEXER_TESTS})
target_include_directories(test_liblexer PUBLIC ${TEST_DEPS} ${LANG_COMMON} ${LANG_SRC_DIR}/liblexer)
target_link_libraries(test_liblexer lexer lang_common GTest::gtest_main)

file(GLOB_RECURSE LIBPARSER_TESTS ${LANG_TEST_DIR}/libparser/*.cpp)
add_executable(test_libparser ${LIBPARSER_TESTS})
target_include_directories(test_libparser PUBLIC ${TEST_DEPS} ${LANG_COMMON} ${LANG_SRC_DIR}/libparser)
target_link_libraries(test_libparser parser lang_common GTest::gtest_main)

# MASTER TESTS
file(GLOB_RECURSE LIBTSM_TESTS ${MASTER_TEST_DIR}/libTSM/*.cpp)
add_executable(test_libTSM ${LIBTSM_TESTS})
target_include_directories(test_libTSM PUBLIC ${TEST_DEPS} ${MASTER_SRC_DIR}/libTSM)
target_link_libraries(test_libTSM TSM GTest::gtest_main)

file(GLOB_RECURSE LIBEM_TESTS ${MASTER_TEST_DIR}/libEM/*.cpp)
add_executable(test_libEM ${LIBEM_TESTS})
target_include_directories(test_libEM PUBLIC ${TEST_DEPS} ${MASTER_SRC_DIR}/libEM)
target_link_libraries(test_libEM EM GTest::gtest_main)

file(GLOB_RECURSE LIBMM_TESTS ${MASTER_TEST_DIR}/libMM/*.cpp)
add_executable(test_libMM ${LIBMM_TESTS})
target_include_directories(test_libMM PUBLIC ${TEST_DEPS} ${MASTER_SRC_DIR}/libMM)
target_link_libraries(test_libMM MM GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(test_liblexer)
gtest_discover_tests(test_libparser)
gtest_discover_tests(test_libTSQ)
gtest_discover_tests(test_libTSM)
gtest_discover_tests(test_libDM)
gtest_discover_tests(test_libEM)
gtest_discover_tests(test_libMM)

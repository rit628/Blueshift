cmake_minimum_required(VERSION 3.29) # for some reason setting 3.30 causes redundant rebuilds

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_SCAN_FOR_MODULES)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_AR llvm-ar)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-sign-compare -Wno-unused-parameter")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -glldb -gfull")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Og -glldb -DNDEBUG")

# Packages
include(FetchContent)
set(FETCHCONTENT_QUIET OFF)

# GoogleTest
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/b514bdc898e2951020cbdca1304b75f5950d1f59.zip
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Boost
set(REQUIRED_BOOST_LIBS asio thread regex lockfree type_index math json serialization)
list(TRANSFORM REQUIRED_BOOST_LIBS PREPEND Boost:: OUTPUT_VARIABLE BOOST_LIB_DEPS)
FetchContent_Declare(
    Boost
    URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.zip
)
set(BOOST_INCLUDE_LIBRARIES ${REQUIRED_BOOST_LIBS})

FetchContent_MakeAvailable(googletest Boost)

project($ENV{PROJECT_NAME})
    
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$ENV{RUNTIME_OUTPUT_DIRECTORY})

set(BLS_CONFIG ${CMAKE_SOURCE_DIR}/config)

set(SRC_DIR ${CMAKE_SOURCE_DIR}/src)
set(UTIL_SRC_DIR ${SRC_DIR}/util)
set(COMMON_SRC_DIR ${SRC_DIR}/common)
set(LANG_SRC_DIR ${SRC_DIR}/lang)
set(MASTER_SRC_DIR ${SRC_DIR}/master)
set(CLIENT_SRC_DIR ${SRC_DIR}/client)


set(TEST_DIR ${CMAKE_SOURCE_DIR}/test)
set(TEST_DEPS ${TEST_DIR}/include)
set(COMMON_TEST_DIR ${TEST_DIR}/common)
set(LANG_TEST_DIR ${TEST_DIR}/lang)
set(MASTER_TEST_DIR ${TEST_DIR}/master)
set(CLIENT_TEST_DIR ${TEST_DIR}/client)

set(LANG_COMMON ${LANG_SRC_DIR}/common)

# MACRO DEFINITIONS
if(NOT CMAKE_CROSSCOMPILING)
    string(TOUPPER ${CMAKE_SYSTEM_NAME} CONTROLLER_TARGET)
    add_compile_definitions(CONTROLLER_TARGET="${CONTROLLER_TARGET}")
endif()
add_compile_definitions(BROADCAST_PORT=$ENV{BROADCAST_PORT})
add_compile_definitions(MASTER_PORT=$ENV{MASTER_PORT})

# COMMON LIBS
file(GLOB_RECURSE LIBTSQ_SRC ${COMMON_SRC_DIR}/libTSQ/*.cpp)
add_library(TSQ STATIC ${LIBTSQ_SRC})

file(GLOB_RECURSE LIBTYPES_SRC ${COMMON_SRC_DIR}/libtypes/*.cpp)
add_library(types STATIC ${LIBTYPES_SRC})
target_include_directories(types PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(types ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBDM_SRC ${COMMON_SRC_DIR}/libDM/*.cpp)
add_library(dynamic_message STATIC ${LIBDM_SRC})
target_include_directories(dynamic_message PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(dynamic_message types ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBNETWORK_SRC ${COMMON_SRC_DIR}/libnetwork/*.cpp)
add_library(network STATIC ${LIBNETWORK_SRC})
target_include_directories(network PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(network TSQ ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBBYTECODE_SRC ${COMMON_SRC_DIR}/libbytecode/*.cpp)
add_library(bytecode STATIC ${LIBBYTECODE_SRC})
target_include_directories(bytecode PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(bytecode ${BOOST_LIB_DEPS})

# LANG LIBS AND EXECUTABLE
file(GLOB_RECURSE LANG_COMMON_SRC ${LANG_COMMON}/*.cpp)
add_library(lang_common STATIC ${LANG_COMMON_SRC})
target_include_directories(lang_common PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(lang_common types ${BOOST_LIB_DEPS})

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
target_include_directories(interpreter PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${LANG_COMMON})
target_link_libraries(interpreter lang_common types ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBANALYZER_SRC ${LANG_SRC_DIR}/libanalyzer/*.cpp)
add_library(analyzer STATIC ${LIBANALYZER_SRC})
target_include_directories(analyzer PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${LANG_COMMON})
target_link_libraries(analyzer lang_common types ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBGENERATOR_SRC ${LANG_SRC_DIR}/libgenerator/*.cpp)
add_library(generator STATIC ${LIBGENERATOR_SRC})
target_include_directories(generator PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${LANG_COMMON})
target_link_libraries(generator lang_common types ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBVIRTUALMACHINE_SRC ${LANG_SRC_DIR}/libvirtual_machine/*.cpp)
add_library(virtual_machine STATIC ${LIBVIRTUALMACHINE_SRC})
target_include_directories(virtual_machine PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${LANG_COMMON})
target_link_libraries(virtual_machine lang_common types bytecode ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBCOMPILER_SRC ${LANG_SRC_DIR}/libcompiler/*.cpp)
add_library(compiler STATIC ${LIBCOMPILER_SRC})
target_include_directories(compiler PUBLIC ${BLS_CONFIG} ${LANG_COMMON} ${LANG_SRC_DIR})
target_link_libraries(compiler lang_common lexer parser interpreter analyzer generator ${BOOST_LIB_DEPS})

add_executable(lang ${LANG_SRC_DIR}/main.cpp)
target_link_libraries(lang compiler virtual_machine ${BOOST_LIB_DEPS})

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
target_link_libraries(EM TSM TSQ dynamic_message interpreter ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBMM_SRC ${MASTER_SRC_DIR}/libMM/*.cpp)
add_library(MM STATIC ${LIBMM_SRC})
target_include_directories(MM PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${MASTER_SRC_DIR})
target_link_libraries(MM EM TSQ dynamic_message ${BOOST_LIB_DEPS})

file(GLOB_RECURSE LIBNM_SRC ${MASTER_SRC_DIR}/libNM/*.cpp)
add_library(NM STATIC ${LIBNM_SRC})
target_include_directories(NM PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${MASTER_SRC_DIR})
target_link_libraries(NM network dynamic_message ticker ${BOOST_LIB_DEPS})

add_executable(master ${MASTER_SRC_DIR}/main.cpp)
target_include_directories(master PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${LANG_SRC_DIR})
target_link_libraries(master TSQ EM MM NM compiler ${BOOST_LIB_DEPS})

# CLIENT LIBS AND EXECUTABLE
file(GLOB_RECURSE DRIVER_SRC ${CLIENT_SRC_DIR}/drivers/*.cpp)
add_library(drivers OBJECT ${DRIVER_SRC}) # must be an object library to ensure strong symbols for controller dependent drivers are utilized
target_include_directories(drivers PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${CLIENT_SRC_DIR}/libDevice)
target_link_libraries(drivers dynamic_message ${BOOST_LIB_DEPS} ${SYSTEM_LINK_LIBRARIES})

file(GLOB_RECURSE LIBDEVICE_SRC ${CLIENT_SRC_DIR}/libDevice/*.cpp)
add_library(device STATIC ${LIBDEVICE_SRC} $<TARGET_OBJECTS:drivers>)
target_include_directories(device PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(device network dynamic_message ${BOOST_LIB_DEPS} ${SYSTEM_LINK_LIBRARIES})

file(GLOB_RECURSE LIBCE_SRC ${CLIENT_SRC_DIR}/libCE/*.cpp)
add_library(client_engine STATIC ${LIBCE_SRC})
target_include_directories(client_engine PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${CLIENT_SRC_DIR})
target_link_libraries(client_engine network device ${BOOST_LIB_DEPS})

add_executable(client ${CLIENT_SRC_DIR}/main.cpp)
target_include_directories(client PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR} ${LANG_SRC_DIR})
target_link_libraries(client "$<LINK_LIBRARY:WHOLE_ARCHIVE,device>" client_engine ${BOOST_LIB_DEPS}) # whole library archive is linked to check for strong symbols

# UTILITY EXECUTABLES
add_executable(echo ${UTIL_SRC_DIR}/echo/main.cpp)

file(GLOB_RECURSE BSMP_SRC ${UTIL_SRC_DIR}/bsmp/*.cpp)
add_executable(bsmp ${BSMP_SRC})
target_include_directories(bsmp PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(bsmp bytecode types ${BOOST_LIB_DEPS})

file(GLOB_RECURSE BSMW_SRC ${UTIL_SRC_DIR}/bsmw/*.cpp)
add_executable(bsmw ${BSMW_SRC})
target_include_directories(bsmw PUBLIC ${BLS_CONFIG} ${COMMON_SRC_DIR})
target_link_libraries(bsmw bytecode types ${BOOST_LIB_DEPS})

# TESTS
if(NOT CMAKE_CROSSCOMPILING)
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
    target_include_directories(test_libparser PUBLIC ${TEST_DEPS} ${LANG_COMMON} ${LANG_SRC_DIR}/libparser ${LANG_TEST_DIR}/include)
    target_link_libraries(test_libparser parser lang_common GTest::gtest_main)

    file(GLOB_RECURSE LIBANALYZER_TESTS ${LANG_TEST_DIR}/libanalyzer/*.cpp)
    add_executable(test_libanalyzer ${LIBANALYZER_TESTS})
    target_include_directories(test_libanalyzer PUBLIC ${TEST_DEPS} ${LANG_COMMON} ${LANG_SRC_DIR}/libanalyzer ${LANG_TEST_DIR}/include)
    target_link_libraries(test_libanalyzer analyzer lang_common GTest::gtest_main)

    file(GLOB_RECURSE LIBGENERATOR_TESTS ${LANG_TEST_DIR}/libgenerator/*.cpp)
    add_executable(test_libgenerator ${LIBGENERATOR_TESTS})
    target_include_directories(test_libgenerator PUBLIC ${TEST_DEPS} ${LANG_COMMON} ${LANG_SRC_DIR}/libgenerator ${LANG_TEST_DIR}/include)
    target_link_libraries(test_libgenerator generator lang_common GTest::gtest_main)

    file(GLOB_RECURSE LANG_E2E_TESTS ${LANG_TEST_DIR}/e2e/*.cpp)
    add_executable(test_lang ${LANG_E2E_TESTS})
    target_include_directories(test_lang PUBLIC ${TEST_DEPS} ${LANG_COMMON} ${LANG_SRC_DIR})
    target_link_libraries(test_lang compiler lang_common GTest::gtest_main)

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
    gtest_discover_tests(test_libanalyzer)
    gtest_discover_tests(test_libgenerator)
    gtest_discover_tests(
        test_lang
        WORKING_DIRECTORY ${LANG_TEST_DIR}/e2e/samples
    )
    gtest_discover_tests(test_libTSQ)
    gtest_discover_tests(test_libTSM)
    gtest_discover_tests(test_libDM)
    gtest_discover_tests(test_libEM)
    gtest_discover_tests(test_libMM)
endif()
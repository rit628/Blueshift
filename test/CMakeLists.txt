if(NOT CMAKE_CROSSCOMPILING)
    include_directories(${TEST_DIR}/include)
    include_directories(${SRC_DIR}/common)

    include(GoogleTest)

    add_subdirectory(common)
    add_subdirectory(lang)
    add_subdirectory(master)
endif()
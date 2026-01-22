if(NOT CMAKE_CROSSCOMPILING)
    include_directories(include)
    
    include(GoogleTest)

    add_subdirectory(common)
    add_subdirectory(lang)
    add_subdirectory(master)
endif()
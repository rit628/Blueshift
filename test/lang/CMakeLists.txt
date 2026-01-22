set(LANG_SRC_DIR ${SRC_DIR}/lang)
include_directories(include)

add_subdirectory(liblexer)
add_subdirectory(libparser)
add_subdirectory(libanalyzer)
add_subdirectory(libgenerator)
add_subdirectory(e2e)
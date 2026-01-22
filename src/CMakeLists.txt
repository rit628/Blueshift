include_directories(common)

set(UTIL_SRC_DIR util)
set(COMMON_SRC_DIR common)
set(LANG_SRC_DIR lang)
set(MASTER_SRC_DIR master)
set(CLIENT_SRC_DIR client)

add_subdirectory(common)
add_subdirectory(util)
add_subdirectory(lang)
add_subdirectory(master)
add_subdirectory(client)
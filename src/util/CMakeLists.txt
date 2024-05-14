set(UTIL_DIRS ${CMAKE_CURRENT_LIST_DIR}/inc)
aux_source_directory(. UTIL_SOURCES)

add_library(Util STATIC ${UTIL_SOURCES})
target_include_directories(Util PUBLIC ${UTIL_DIRS})

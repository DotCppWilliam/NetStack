set(NET_DIRS ${CMAKE_CURRENT_LIST_DIR}/inc)
aux_source_directory(. NET_SOURCES)

add_library(Net STATIC ${NET_SOURCES})
target_include_directories(Net PUBLIC ${NET_DIRS})
target_link_libraries(Net PUBLIC Util Plat)



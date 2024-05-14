set(DIRS ${CMAKE_CURRENT_LIST_DIR}/inc)
aux_source_directory(. SOURCES)

add_library(Pcap STATIC ${SOURCES})
target_include_directories(Pcap PUBLIC ${DIRS})
target_link_libraries(Pcap PRIVATE ${CMAKE_CURRENT_LIST_DIR}/libpcap.a dbus-1)

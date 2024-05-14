set(PLAT_DIRS ${CMAKE_CURRENT_LIST_DIR}/inc)
SET(PCAP_DIRS ${PROJECT_SOURCE_DIR}/pcap/inc)
aux_source_directory(. PLAT_SOURCES)

add_library(Plat STATIC ${PLAT_SOURCES})
target_include_directories(Plat PUBLIC ${PLAT_DIRS})
target_include_directories(Plat PUBLIC ${PCAP_DIRS})
target_link_libraries(Plat PRIVATE ${PROJECT_SOURCE_DIR}/src/pcap/libpcap.a dbus-1)
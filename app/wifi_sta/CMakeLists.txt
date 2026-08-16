cmake_minimum_required(VERSION 3.20.0)

if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf.local)
    list(APPEND OVERLAY_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/prj.conf.local)
endif()

find_package(Zephyr REQUIRED)
project(wifi_sta)


target_sources(app PRIVATE src/main.c)

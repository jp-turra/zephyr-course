cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED)
project(rpi_usb_console)

target_sources(app PRIVATE src/main.cpp)

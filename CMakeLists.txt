cmake_minimum_required(VERSION 3.10)
project(FAT32Emulator C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

file(GLOB SOURCES "src/*.c")
add_executable(fat32_emulator ${SOURCES})

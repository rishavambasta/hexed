cmake_minimum_required(VERSION 3.10)
project(hxd C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

include_directories(
    src
    src/core
    src/ui
    ${GTK3_INCLUDE_DIRS}
)

link_directories(${GTK3_LIBRARY_DIRS})
add_definitions(${GTK3_CFLAGS_OTHER})

file(GLOB_RECURSE CORE_SRCS "src/core/*.c")
file(GLOB_RECURSE UI_SRCS "src/ui/*.c")

add_executable(hxd
    src/main.c
    ${CORE_SRCS}
    ${UI_SRCS}
)

target_link_libraries(hxd
    ${GTK3_LIBRARIES}
    m
)

# Tests
enable_testing()
add_executable(test_piece_table tests/test_piece_table.c src/core/piece_table.c)
add_test(NAME piece_table COMMAND test_piece_table)

add_executable(test_hex_records tests/test_hex_records.c src/core/intel_hex.c src/core/srec.c src/core/piece_table.c)
add_test(NAME hex_records COMMAND test_hex_records)

add_executable(test_core_all tests/test_core_all.c ${CORE_SRCS})
target_link_libraries(test_core_all m)
add_test(NAME core_all COMMAND test_core_all)

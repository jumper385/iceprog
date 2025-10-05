# Build the gtk based gui for windows
cmake_minimum_required(VERSION 3.10)
project(iceprog_gui C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

add_executable(iceprog_gui gui.c)

target_include_directories(iceprog_gui PRIVATE ${GTK3_INCLUDE_DIRS})
target_compile_options(iceprog_gui PRIVATE ${GTK3_CFLAGS_OTHER})
target_link_directories(iceprog_gui PRIVATE ${GTK3_LIBRARY_DIRS})
target_link_libraries(iceprog_gui PRIVATE ${GTK3_LIBRARIES})
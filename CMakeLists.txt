cmake_minimum_required(VERSION 3.16)
project(iceprog_gui C)

option(USE_GTK "Force GTK build even on Windows" OFF)

if (WIN32 AND NOT USE_GTK)
    add_executable(iceprog_gui WIN32 gui_win32.c)
    target_compile_definitions(iceprog_gui PRIVATE UNICODE _UNICODE)
    target_link_libraries(iceprog_gui PRIVATE user32)
else()
  find_package(PkgConfig REQUIRED)
  # IMPORTED_TARGET gives you an interface target with proper include + link dirs
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)

  add_executable(iceprog_gui gui.c)
  target_link_libraries(iceprog_gui PRIVATE PkgConfig::GTK3)
endif()
cmake_minimum_required(VERSION 3.16)
project(iceprog_gui C)

option(USE_GTK "Force GTK build even on Windows" OFF)

if (WIN32 AND NOT USE_GTK)
    # Windows build with Win32 API
    add_executable(iceprog_gui WIN32 gui_win32.c iceprog_fn.c mpsse.c)
    
    # Link Windows system libraries
    target_link_libraries(iceprog_gui PRIVATE 
        user32 
        gdi32 
        comdlg32 
        comctl32
    )
    
    # Try to find and link libftdi
    find_library(FTDI_LIBRARY ftdi1)
    if(FTDI_LIBRARY)
        target_link_libraries(iceprog_gui PRIVATE ${FTDI_LIBRARY})
    else()
        # Try alternative names for libftdi
        find_library(FTDI_LIBRARY_ALT ftdi)
        if(FTDI_LIBRARY_ALT)
            target_link_libraries(iceprog_gui PRIVATE ${FTDI_LIBRARY_ALT})
        else()
            message(WARNING "libftdi not found. Make sure it's installed and in your PATH.")
        endif()
    endif()
    
else()
  find_package(PkgConfig REQUIRED)
  # IMPORTED_TARGET gives you an interface target with proper include + link dirs
  pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
  
  # Also need libftdi for the MPSSE functionality
  pkg_check_modules(LIBFTDI REQUIRED IMPORTED_TARGET libftdi1)

  add_executable(iceprog_gui gui.c iceprog_fn.c mpsse.c)
  target_link_libraries(iceprog_gui PRIVATE PkgConfig::GTK3 PkgConfig::LIBFTDI)
endif()
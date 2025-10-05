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
        ws2_32
    )
    
    # Try to find and link libftdi - try different possible locations and names
    find_path(FTDI_INCLUDE_DIR
        NAMES ftdi.h libftdi1/ftdi.h
        PATHS
            ${CMAKE_PREFIX_PATH}/include
            $ENV{PROGRAMFILES}/libftdi/include
            $ENV{PROGRAMFILES\(X86\)}/libftdi/include
            C:/vcpkg/installed/x64-windows/include
            C:/vcpkg/installed/x86-windows/include
    )
    
    find_library(FTDI_LIBRARY
        NAMES ftdi1 ftdi libftdi1 libftdi
        PATHS
            ${CMAKE_PREFIX_PATH}/lib
            $ENV{PROGRAMFILES}/libftdi/lib
            $ENV{PROGRAMFILES\(X86\)}/libftdi/lib
            C:/vcpkg/installed/x64-windows/lib
            C:/vcpkg/installed/x86-windows/lib
    )
    
    if(FTDI_INCLUDE_DIR)
        target_include_directories(iceprog_gui PRIVATE ${FTDI_INCLUDE_DIR})
        message(STATUS "Found libftdi headers at: ${FTDI_INCLUDE_DIR}")
        # Check if we're using the libftdi1 subdirectory structure
        if(EXISTS "${FTDI_INCLUDE_DIR}/libftdi1/ftdi.h")
            target_compile_definitions(iceprog_gui PRIVATE HAVE_LIBFTDI1_FTDI_H)
        endif()
    else()
        message(WARNING "libftdi headers not found. Please install libftdi1-dev or set CMAKE_PREFIX_PATH")
    endif()
    
    if(FTDI_LIBRARY)
        target_link_libraries(iceprog_gui PRIVATE ${FTDI_LIBRARY})
        message(STATUS "Found libftdi library at: ${FTDI_LIBRARY}")
    else()
        message(WARNING "libftdi library not found. Please install libftdi1 or set CMAKE_PREFIX_PATH")
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
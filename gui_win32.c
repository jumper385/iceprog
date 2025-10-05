#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// Force ANSI/ASCII string functions
#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif

// include iceprog library
#include "iceprog_fn.h"
#include "mpsse.h"

// Link required libraries
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "comctl32.lib")

// Windows compatibility - Sleep is in milliseconds, usleep was in microseconds
#define usleep(x) Sleep((x)/1000)

// Window controls IDs
#define ID_BTN_TEST_CONNECTION  1001
#define ID_BTN_SELECT_FILE      1002
#define ID_BTN_FLASH_CHIP       1003
#define ID_LBL_FILE_PATH        1004
#define ID_PROGRESS_BAR         1005

// Window dimensions
#define WINDOW_WIDTH    500
#define WINDOW_HEIGHT   300
#define BUTTON_WIDTH    200
#define BUTTON_HEIGHT   30
#define MARGIN          10

// Global variables
static bool mpsse_initialized = false;
static char selected_file_path[MAX_PATH] = {0};
static HWND hwnd_main = NULL;
static HWND hwnd_lbl_file_path = NULL;
static HWND hwnd_progress_bar = NULL;
static HWND hwnd_btn_test = NULL;
static HWND hwnd_btn_select = NULL;
static HWND hwnd_btn_flash = NULL;
static FILE *log_file = NULL;

// Function prototypes
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void CreateControls(HWND hwnd);
void OnTestConnection(void);
void OnSelectFile(HWND hwnd);
void OnFlashChip(void);
void UpdateProgress(int percent, const char *text);
void CleanupMpsse(void);
void InitLogging(void);
void LogMessage(const char *format, ...);
void CloseLogging(void);

void InitLogging(void) {
    // Create log file immediately with basic error handling
    log_file = fopen("iceprog_debug.log", "w");
    if (!log_file) {
        log_file = fopen("C:\\temp\\iceprog_debug.log", "w");
    }
    if (!log_file) {
        // Try current directory as last resort
        log_file = fopen(".\\iceprog_debug.log", "w");
    }
    
    if (log_file) {
        fprintf(log_file, "=== IceProg GUI Debug Log Started ===\n");
        fflush(log_file);
    }
    
    // Try to create console - if this fails, we'll still have the log file
    if (AllocConsole()) {
        freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
        freopen_s((FILE**)stderr, "CONOUT$", "w", stderr);
        printf("Debug console opened successfully\n");
    }
    
    // Write to debug log immediately
    if (log_file) {
        fprintf(log_file, "InitLogging completed successfully\n");
        fflush(log_file);
    }
}

void LogMessage(const char *format, ...) {
    va_list args;
    char buffer[1024];
    SYSTEMTIME st;
    
    GetLocalTime(&st);
    
    // Format the message
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Write to log file with timestamp
    if (log_file) {
        fprintf(log_file, "[%04d-%02d-%02d %02d:%02d:%02d.%03d] %s\n",
                st.wYear, st.wMonth, st.wDay,
                st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
                buffer);
        fflush(log_file);
    }
    
    // Also write to console and Visual Studio output
    printf("[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, buffer);
    
    // Write to debug output (visible in Visual Studio)
    char debug_buffer[1100];
    snprintf(debug_buffer, sizeof(debug_buffer), "[IceProg] %s\n", buffer);
    OutputDebugStringA(debug_buffer);
}

void CloseLogging(void) {
    if (log_file) {
        LogMessage("=== IceProg GUI Log Ended ===");
        fclose(log_file);
        log_file = NULL;
    }
}

static void update_progress(double fraction, const char *text) {
    if (hwnd_progress_bar) {
        int percent = (int)(fraction * 100);
        SendMessage(hwnd_progress_bar, PBM_SETPOS, percent, 0);
        SetWindowTextA(hwnd_progress_bar, text);
        
        // Process pending messages to update the display
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

static void cleanup_mpsse() {
    LogMessage("=== Cleanup Started ===");
    
    if (mpsse_initialized) {
        LogMessage("Cleaning up MPSSE interface...");
        __try {
            mpsse_close();
            LogMessage("mpsse_close completed successfully");
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            LogMessage("Exception during mpsse_close");
        }
        mpsse_initialized = false;
    }
    
    memset(selected_file_path, 0, sizeof(selected_file_path));
    LogMessage("=== Cleanup Completed ===");
}

void OnTestConnection(void) {
    LogMessage("=== Test Connection Started ===");
    
    __try {
        LogMessage("Testing SPI Flash connection...");
        
        // Initialize MPSSE if not already done
        if (!mpsse_initialized) {
            LogMessage("Initializing MPSSE interface...");
            
            // Use default parameters: interface 0, no device string, normal clock speed
            LogMessage("Calling mpsse_init(0, NULL, false)...");
            mpsse_init(0, NULL, false);
            LogMessage("mpsse_init completed successfully");
            
            mpsse_initialized = true;
            LogMessage("MPSSE initialized successfully");
            
            // Release reset and setup flash
            LogMessage("Calling flash_release_reset()...");
            flash_release_reset();
            LogMessage("flash_release_reset completed");
            
            LogMessage("Sleeping for 100ms...");
            usleep(100000);  // 100ms
            LogMessage("Sleep completed");
            
            LogMessage("Flash reset released");
        } else {
            LogMessage("MPSSE already initialized, skipping initialization");
        }
        
        // Test the flash connection
        LogMessage("Reading flash ID...");
        
        LogMessage("Calling flash_reset()...");
        flash_reset();
        LogMessage("flash_reset completed");
        
        LogMessage("Calling flash_power_up()...");
        flash_power_up();
        LogMessage("flash_power_up completed");
        
        LogMessage("Calling flash_read_id()...");
        flash_read_id();
        LogMessage("flash_read_id completed");
        
        LogMessage("Calling flash_power_down()...");
        flash_power_down();
        LogMessage("flash_power_down completed");
        
        LogMessage("Flash test completed successfully");
        
        MessageBoxA(hwnd_main, "Flash test completed successfully!\nCheck iceprog_gui.log for details.", "Test Connection", MB_OK | MB_ICONINFORMATION);
        
    } __except(GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : 
               GetExceptionCode() == EXCEPTION_INT_DIVIDE_BY_ZERO ? EXCEPTION_EXECUTE_HANDLER :
               EXCEPTION_EXECUTE_HANDLER) {
        LogMessage("EXCEPTION CAUGHT in OnTestConnection!");
        
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
            "An exception occurred during flash test!\nCheck iceprog_gui.log for details.");
        MessageBoxA(hwnd_main, error_msg, "Error", MB_OK | MB_ICONERROR);
    }
    
    LogMessage("=== Test Connection Ended ===");
}

void OnSelectFile(HWND hwnd) {
    printf("Selecting bitstream file...\n");
    
    OPENFILENAMEA ofn;
    char szFile[MAX_PATH] = {0};
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "Bitstream Files\0*.bin;*.bit\0All Files\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    
    if (GetOpenFileNameA(&ofn)) {
        strcpy_s(selected_file_path, sizeof(selected_file_path), szFile);
        printf("Selected file: %s\n", selected_file_path);
        
        // Update the label to show the selected file
        if (hwnd_lbl_file_path) {
            SetWindowTextA(hwnd_lbl_file_path, selected_file_path);
        }
    }
}

void OnFlashChip(void) {
    printf("Flashing the chip...\n");
    
    // Check if a file is selected
    if (strlen(selected_file_path) == 0) {
        printf("Error: No bitstream file selected!\n");
        MessageBoxA(hwnd_main, "Please select a bitstream file first!", "Error", MB_OK | MB_ICONERROR);
        update_progress(0.0, "Error: No file selected");
        return;
    }
    
    update_progress(0.0, "Opening file...");
    
    // Open the selected file
    FILE *f = fopen(selected_file_path, "rb");
    if (f == NULL) {
        printf("Error: Cannot open file '%s' for reading\n", selected_file_path);
        MessageBoxA(hwnd_main, "Cannot open the selected file!", "Error", MB_OK | MB_ICONERROR);
        update_progress(0.0, "Error: Cannot open file");
        return;
    }
    
    // Get file size
    fseek(f, 0L, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0L, SEEK_SET);
    
    if (file_size <= 0) {
        printf("Error: Invalid file size\n");
        MessageBoxA(hwnd_main, "Invalid file size!", "Error", MB_OK | MB_ICONERROR);
        update_progress(0.0, "Error: Invalid file size");
        fclose(f);
        return;
    }
    
    printf("File size: %ld bytes\n", file_size);
    update_progress(0.05, "File loaded successfully");
    
    // Initialize MPSSE if not already done
    if (!mpsse_initialized) {
        update_progress(0.1, "Initializing MPSSE interface...");
        printf("Initializing MPSSE interface...\n");
        mpsse_init(0, NULL, false);
        mpsse_initialized = true;
        flash_release_reset();
        usleep(100000);  // 100ms
    }
    
    // Reset and prepare flash
    update_progress(0.15, "Preparing flash...");
    printf("Preparing flash...\n");
    flash_chip_deselect();
    usleep(250000);  // 250ms
    flash_reset();
    flash_power_up();
    flash_read_id();
    
    // Erase flash (using 64kB sectors)
    update_progress(0.2, "Erasing flash...");
    printf("Erasing flash...\n");
    int erase_block_size = 64; // 64kB sectors
    int block_size = erase_block_size << 10; // Convert to bytes
    int block_mask = block_size - 1;
    int begin_addr = 0 & ~block_mask;
    int end_addr = (file_size + block_mask) & ~block_mask;
    
    int total_erase_blocks = (end_addr - begin_addr) / block_size;
    int current_erase_block = 0;
    
    for (int addr = begin_addr; addr < end_addr; addr += block_size) {
        double erase_progress = 0.2 + (0.3 * current_erase_block / total_erase_blocks);
        char erase_text[100];
        snprintf(erase_text, sizeof(erase_text), "Erasing sector %d/%d", 
                current_erase_block + 1, total_erase_blocks);
        update_progress(erase_progress, erase_text);
        
        printf("Erasing sector at 0x%06X\n", addr);
        flash_write_enable();
        flash_64kB_sector_erase(addr);
        flash_wait();
        current_erase_block++;
    }
    
    // Program flash
    update_progress(0.5, "Programming flash...");
    printf("Programming flash...\n");
    for (int rc, addr = 0; true; addr += rc) {
        uint8_t buffer[256];
        int page_size = 256 - addr % 256;
        rc = fread(buffer, 1, page_size, f);
        if (rc <= 0)
            break;
            
        double prog_progress = 0.5 + (0.3 * addr / file_size);
        char prog_text[100];
        snprintf(prog_text, sizeof(prog_text), "Programming: %ld%% (0x%06X)", 
                100 * addr / file_size, addr);
        update_progress(prog_progress, prog_text);
        
        flash_write_enable();
        flash_prog(addr, buffer, rc);
        flash_wait();
    }
    
    // Verify programming
    update_progress(0.8, "Verifying flash...");
    printf("Verifying flash...\n");
    fseek(f, 0, SEEK_SET);
    bool verify_ok = true;
    for (int addr = 0; true; addr += 256) {
        uint8_t buffer_flash[256], buffer_file[256];
        int rc = fread(buffer_file, 1, 256, f);
        if (rc <= 0)
            break;
            
        double verify_progress = 0.8 + (0.15 * addr / file_size);
        char verify_text[100];
        snprintf(verify_text, sizeof(verify_text), "Verifying: %ld%% (0x%06X)", 
                100 * addr / file_size, addr);
        update_progress(verify_progress, verify_text);
        
        flash_read(addr, buffer_flash, rc);
        if (memcmp(buffer_file, buffer_flash, rc)) {
            printf("\nVerification failed at address 0x%06X!\n", addr);
            update_progress(0.95, "Verification failed!");
            verify_ok = false;
            break;
        }
    }
    
    // Power down flash
    update_progress(0.95, "Finalizing...");
    flash_power_down();
    flash_release_reset();
    usleep(250000);  // 250ms
    
    fclose(f);
    
    if (verify_ok) {
        printf("\nVERIFY OK\n");
        update_progress(1.0, "Flash completed successfully!");
        printf("Flash operation completed.\n");
        MessageBoxA(hwnd_main, "Flash operation completed successfully!", "Success", MB_OK | MB_ICONINFORMATION);
    } else {
        update_progress(0.0, "Flash failed - verification error");
        MessageBoxA(hwnd_main, "Flash failed - verification error!", "Error", MB_OK | MB_ICONERROR);
    }
}

void CreateControls(HWND hwnd) {
    // Initialize common controls
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_PROGRESS_CLASS;
    InitCommonControlsEx(&icc);
    
    int y_pos = MARGIN;
    
    // Test connection button
    hwnd_btn_test = CreateWindowA(
        "BUTTON", "Test Probe Connection",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        (WINDOW_WIDTH - BUTTON_WIDTH) / 2, y_pos,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)ID_BTN_TEST_CONNECTION, GetModuleHandle(NULL), NULL);
    y_pos += BUTTON_HEIGHT + MARGIN;
    
    // Select file button
    hwnd_btn_select = CreateWindowA(
        "BUTTON", "Select Bitstream File",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        (WINDOW_WIDTH - BUTTON_WIDTH) / 2, y_pos,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)ID_BTN_SELECT_FILE, GetModuleHandle(NULL), NULL);
    y_pos += BUTTON_HEIGHT + MARGIN;
    
    // File path label
    hwnd_lbl_file_path = CreateWindowA(
        "STATIC", "No file selected",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        MARGIN, y_pos,
        WINDOW_WIDTH - 2 * MARGIN, 20,
        hwnd, (HMENU)ID_LBL_FILE_PATH, GetModuleHandle(NULL), NULL);
    y_pos += 25 + MARGIN;
    
    // Progress bar
    hwnd_progress_bar = CreateWindowA(
        PROGRESS_CLASSA, NULL,
        WS_VISIBLE | WS_CHILD | PBS_SMOOTH,
        MARGIN, y_pos,
        WINDOW_WIDTH - 2 * MARGIN, 25,
        hwnd, (HMENU)ID_PROGRESS_BAR, GetModuleHandle(NULL), NULL);
    
    // Set progress bar range
    SendMessage(hwnd_progress_bar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(hwnd_progress_bar, PBM_SETPOS, 0, 0);
    y_pos += 30 + MARGIN;
    
    // Flash chip button
    hwnd_btn_flash = CreateWindowA(
        "BUTTON", "Flash Chip",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        (WINDOW_WIDTH - BUTTON_WIDTH) / 2, y_pos,
        BUTTON_WIDTH, BUTTON_HEIGHT,
        hwnd, (HMENU)ID_BTN_FLASH_CHIP, GetModuleHandle(NULL), NULL);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            LogMessage("WM_CREATE received, creating controls...");
            CreateControls(hwnd);
            LogMessage("Controls created successfully");
            return 0;
            
        case WM_COMMAND:
            LogMessage("WM_COMMAND received, command ID: %d", LOWORD(wParam));
            switch (LOWORD(wParam)) {
                case ID_BTN_TEST_CONNECTION:
                    LogMessage("Test connection button clicked");
                    OnTestConnection();
                    break;
                case ID_BTN_SELECT_FILE:
                    LogMessage("Select file button clicked");
                    OnSelectFile(hwnd);
                    break;
                case ID_BTN_FLASH_CHIP:
                    LogMessage("Flash chip button clicked");
                    OnFlashChip();
                    break;
            }
            return 0;
            
        case WM_DESTROY:
            LogMessage("WM_DESTROY received, cleaning up...");
            cleanup_mpsse();
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Emergency logging - write directly to file before any complex initialization
    FILE *emergency_log = fopen("emergency_debug.log", "w");
    if (emergency_log) {
        fprintf(emergency_log, "WinMain started successfully\n");
        fprintf(emergency_log, "hInstance: %p\n", hInstance);
        fprintf(emergency_log, "nCmdShow: %d\n", nCmdShow);
        fflush(emergency_log);
        fclose(emergency_log);
    }
    
    // Initialize logging first
    InitLogging();
    LogMessage("=== Application Starting ===");
    LogMessage("Command line: %s", lpCmdLine ? lpCmdLine : "(empty)");
    
    // Register the window class
    const char CLASS_NAME[] = "IceProgGUI";
    
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    LogMessage("Registering window class...");
    if (!RegisterClassA(&wc)) {
        LogMessage("Failed to register window class! Error: %d", GetLastError());
        CloseLogging();
        return 1;
    }
    LogMessage("Window class registered successfully");
    
    // Create the window
    LogMessage("Creating main window...");
    hwnd_main = CreateWindowExA(
        0,                              // Optional window styles
        CLASS_NAME,                     // Window class
        "IceProg GUI - Windows",        // Window text
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, // Window style (fixed size)
        CW_USEDEFAULT, CW_USEDEFAULT,   // Position
        WINDOW_WIDTH, WINDOW_HEIGHT,    // Size
        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
    );
    
    if (hwnd_main == NULL) {
        LogMessage("Failed to create window! Error: %d", GetLastError());
        CloseLogging();
        return 1;
    }
    LogMessage("Main window created successfully");
    
    ShowWindow(hwnd_main, nCmdShow);
    UpdateWindow(hwnd_main);
    LogMessage("Window shown and updated");
    
    // Run the message loop
    LogMessage("Entering message loop...");
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    LogMessage("Message loop ended");
    LogMessage("=== Application Ending ===");
    CloseLogging();
    return 0;
}
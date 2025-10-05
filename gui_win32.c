#define UNICODE
#define _UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

// Control IDs
#define BTN_BROWSE_ID       1001
#define BTN_PROGRAM_ID      1002
#define BTN_READ_ID         1003
#define BTN_VERIFY_ID       1004
#define BTN_TEST_ID         1005
#define EDIT_FILEPATH_ID    1006
#define STATIC_STATUS_ID    1007
#define PROGRESS_ID         1008

// Window dimensions
#define WINDOW_WIDTH  600
#define WINDOW_HEIGHT 400

// Global variables
HWND g_hEdit;
HWND g_hStatus;
HWND g_hProgress;
wchar_t g_selectedFile[MAX_PATH] = L"";

// Function declarations
void BrowseForFile(HWND hwnd);
void UpdateStatus(const wchar_t* message);
void ProgramFile(HWND hwnd);
void ReadFlash(HWND hwnd);
void VerifyFile(HWND hwnd);
void TestConnection(HWND hwnd);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

    switch (msg)
    {
    case WM_CREATE:
        {
            HINSTANCE hInst = ((LPCREATESTRUCT)lParam)->hInstance;
            
            // File path label
            CreateWindowExW(0, L"STATIC", L"File Path:",
                WS_CHILD | WS_VISIBLE,
                20, 20, 80, 20,
                hwnd, NULL, hInst, NULL);
            
            // File path edit box
            g_hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE,
                20, 45, 400, 25,
                hwnd, (HMENU)EDIT_FILEPATH_ID, hInst, NULL);
            
            // Browse button
            CreateWindowExW(0, L"BUTTON", L"Browse...",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                430, 45, 80, 25,
                hwnd, (HMENU)BTN_BROWSE_ID, hInst, NULL);
            
            // Operation buttons
            CreateWindowExW(0, L"BUTTON", L"Program Flash",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                20, 90, 120, 35,
                hwnd, (HMENU)BTN_PROGRAM_ID, hInst, NULL);
            
            CreateWindowExW(0, L"BUTTON", L"Read Flash",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                150, 90, 120, 35,
                hwnd, (HMENU)BTN_READ_ID, hInst, NULL);
            
            CreateWindowExW(0, L"BUTTON", L"Verify",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                280, 90, 120, 35,
                hwnd, (HMENU)BTN_VERIFY_ID, hInst, NULL);
            
            CreateWindowExW(0, L"BUTTON", L"Test Connection",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                410, 90, 120, 35,
                hwnd, (HMENU)BTN_TEST_ID, hInst, NULL);
            
            // Status label
            CreateWindowExW(0, L"STATIC", L"Status:",
                WS_CHILD | WS_VISIBLE,
                20, 145, 60, 20,
                hwnd, NULL, hInst, NULL);
            
            // Status display
            g_hStatus = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"Ready",
                WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                20, 170, 510, 180,
                hwnd, (HMENU)STATIC_STATUS_ID, hInst, NULL);
        }
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case BTN_BROWSE_ID:
            if (HIWORD(wParam) == BN_CLICKED)
                BrowseForFile(hwnd);
            break;
            
        case BTN_PROGRAM_ID:
            if (HIWORD(wParam) == BN_CLICKED)
                ProgramFile(hwnd);
            break;
            
        case BTN_READ_ID:
            if (HIWORD(wParam) == BN_CLICKED)
                ReadFlash(hwnd);
            break;
            
        case BTN_VERIFY_ID:
            if (HIWORD(wParam) == BN_CLICKED)
                VerifyFile(hwnd);
            break;
            
        case BTN_TEST_ID:
            if (HIWORD(wParam) == BN_CLICKED)
                TestConnection(hwnd);
            break;
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR cmd, int nShow)
{
    const wchar_t *CLASS_NAME = L"BasicWin32HelloBye";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc))
        return 1;

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Iceprog GUI",
        WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME, // small fixed window
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInst, NULL);
    if (!hwnd)
        return 1;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

// Function implementations

void BrowseForFile(HWND hwnd)
{
    // For now, just focus the edit control and show helpful message
    SetFocus(g_hEdit);
    UpdateStatus(L"Please enter the file path manually in the text box above (e.g., firmware.bin)");
    MessageBoxW(hwnd, L"Please type or paste the file path into the text box above.\n\nExample: C:\\path\\to\\your\\firmware.bin", 
               L"File Selection", MB_OK | MB_ICONINFORMATION);
}

void UpdateStatus(const wchar_t* message)
{
    if (g_hStatus)
    {
        // Get current text
        int textLen = GetWindowTextLengthW(g_hStatus);
        wchar_t* currentText = (wchar_t*)malloc((textLen + 1) * sizeof(wchar_t));
        if (currentText)
        {
            GetWindowTextW(g_hStatus, currentText, textLen + 1);
            
            // Create new text with timestamp
            wchar_t newText[2048];
            SYSTEMTIME st;
            GetLocalTime(&st);
            
            _snwprintf(newText, 2048, L"%s[%02d:%02d:%02d] %s\r\n", 
                      currentText, st.wHour, st.wMinute, st.wSecond, message);
            
            SetWindowTextW(g_hStatus, newText);
            
            // Scroll to bottom
            SendMessageW(g_hStatus, EM_SETSEL, -1, -1);
            SendMessageW(g_hStatus, EM_SCROLLCARET, 0, 0);
            
            free(currentText);
        }
    }
}

void ProgramFile(HWND hwnd)
{
    // Get file path from edit control
    GetWindowTextW(g_hEdit, g_selectedFile, MAX_PATH);
    
    if (wcslen(g_selectedFile) == 0)
    {
        MessageBoxW(hwnd, L"Please enter a file path first", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    UpdateStatus(L"Starting flash programming...");
    
    // Convert Unicode to ANSI for system call
    char filename[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, g_selectedFile, -1, filename, MAX_PATH, NULL, NULL);
    
    // Build command
    char command[512];
    snprintf(command, 512, "iceprog \"%s\"", filename);
    
    UpdateStatus(L"Executing iceprog command...");
    
    // Execute iceprog
    int result = system(command);
    
    if (result == 0)
    {
        UpdateStatus(L"Programming completed successfully!");
    }
    else
    {
        UpdateStatus(L"Programming failed. Check connection and file.");
    }
}

void ReadFlash(HWND hwnd)
{
    wchar_t szFile[MAX_PATH] = L"flash_dump.bin";
    
    // For simplicity, prompt user for output filename
    if (MessageBoxW(hwnd, L"This will read the flash to 'flash_dump.bin' in the current directory.\n\nProceed?", 
                   L"Read Flash", MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        UpdateStatus(L"Starting flash read...");
        
        // Execute iceprog read command
        int result = system("iceprog -r flash_dump.bin");
        
        if (result == 0)
        {
            UpdateStatus(L"Flash read completed successfully! Saved to flash_dump.bin");
        }
        else
        {
            UpdateStatus(L"Flash read failed. Check connection.");
        }
    }
}

void VerifyFile(HWND hwnd)
{
    // Get file path from edit control
    GetWindowTextW(g_hEdit, g_selectedFile, MAX_PATH);
    
    if (wcslen(g_selectedFile) == 0)
    {
        MessageBoxW(hwnd, L"Please enter a file path first", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    UpdateStatus(L"Starting verification...");
    
    // Convert Unicode to ANSI for system call
    char filename[MAX_PATH];
    WideCharToMultiByte(CP_ACP, 0, g_selectedFile, -1, filename, MAX_PATH, NULL, NULL);
    
    // Build command
    char command[512];
    snprintf(command, 512, "iceprog -c \"%s\"", filename);
    
    UpdateStatus(L"Executing iceprog verify command...");
    
    // Execute iceprog
    int result = system(command);
    
    if (result == 0)
    {
        UpdateStatus(L"Verification completed successfully!");
    }
    else
    {
        UpdateStatus(L"Verification failed. Flash content doesn't match file.");
    }
}

void TestConnection(HWND hwnd)
{
    UpdateStatus(L"Testing connection...");
    
    // Execute iceprog test command
    int result = system("iceprog -t");
    
    if (result == 0)
    {
        UpdateStatus(L"Connection test successful! Device detected.");
    }
    else
    {
        UpdateStatus(L"Connection test failed. Check USB connection and drivers.");
    }
}

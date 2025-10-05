#define UNICODE
#define _UNICODE
#include <windows.h>

#define BTN_ID 1001

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static int isHello = 1;

    switch (msg) {
    case WM_CREATE:
        CreateWindowExW(
            0, L"BUTTON", L"Hello",
            WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
            20, 20, 140, 40,
            hwnd, (HMENU)BTN_ID, ((LPCREATESTRUCT)lParam)->hInstance, NULL
        );
        return 0;

    case WM_COMMAND:
        if (LOWORD(wParam) == BTN_ID && HIWORD(wParam) == BN_CLICKED) {
            HWND btn = GetDlgItem(hwnd, BTN_ID);
            if (btn) {
                isHello = !isHello;
                SetWindowTextW(btn, isHello ? L"Hello" : L"Bye");
            }
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrev, PWSTR cmd, int nShow) {
    const wchar_t *CLASS_NAME = L"BasicWin32HelloBye";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    if (!RegisterClassW(&wc)) return 1;

    HWND hwnd = CreateWindowExW(
        0, CLASS_NAME, L"Win32 – Hello/Bye",
        WS_OVERLAPPEDWINDOW ^ WS_MAXIMIZEBOX ^ WS_THICKFRAME, // small fixed window
        CW_USEDEFAULT, CW_USEDEFAULT, 320, 160,
        NULL, NULL, hInst, NULL
    );
    if (!hwnd) return 1;

    ShowWindow(hwnd, nShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}

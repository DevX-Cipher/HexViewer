#include "die_downloaddialog.h"
#include "die_database.h"

#ifdef _WIN32
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

HWND DIEDownloadDialog::progressWindow = nullptr;
HWND DIEDownloadDialog::progressBar = nullptr;
HWND DIEDownloadDialog::statusText = nullptr;

void DIEDownloadDialog::ProgressCallback(const char* message, int percent)
{
    if (statusText)
    {
        SetWindowTextA(statusText, message);
    }
    
    if (progressBar)
    {
        SendMessage(progressBar, PBM_SETPOS, (WPARAM)percent, 0);
    }
    
    MSG msg;
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

LRESULT CALLBACK DownloadDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CLOSE:
        return 0;
        
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

bool DIEDownloadDialog::Show(HWND parent, bool darkMode)
{
    const char* className = "DIEDownloadDialog";
    
    WNDCLASSA wc = {};
    wc.lpfnWndProc = DownloadDialogProc;
    wc.hInstance = GetModuleHandleA(nullptr);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    UnregisterClassA(className, wc.hInstance);
    if (!RegisterClassA(&wc))
        return false;
    
    int width = 400;
    int height = 150;
    
    RECT parentRect;
    GetWindowRect((HWND)parent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
    
    progressWindow = CreateWindowExA(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        className,
        "Downloading DIE Databases",
        WS_POPUP | WS_CAPTION | WS_SYSMENU,
        x, y, width, height,
        (HWND)parent,
        nullptr,
        wc.hInstance,
        nullptr);
    
    if (!progressWindow)
        return false;
    
    statusText = CreateWindowExA(
        0,
        "STATIC",
        "Initializing download...",
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        10, 20, width - 20, 20,
        progressWindow,
        nullptr,
        wc.hInstance,
        nullptr);
    
    InitCommonControls();
    
    progressBar = CreateWindowExA(
        0,
        PROGRESS_CLASSA,
        nullptr,
        WS_CHILD | WS_VISIBLE,
        10, 50, width - 20, 30,
        progressWindow,
        nullptr,
        wc.hInstance,
        nullptr);
    
    SendMessage(progressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
    SendMessage(progressBar, PBM_SETSTEP, 1, 0);
    
    ShowWindow(progressWindow, SW_SHOW);
    UpdateWindow(progressWindow);
    
    EnableWindow((HWND)parent, FALSE);
    
    bool success = DownloadDIEDatabases(ProgressCallback);
    
    EnableWindow((HWND)parent, TRUE);
    SetForegroundWindow((HWND)parent);
    
    DestroyWindow(progressWindow);
    UnregisterClassA(className, wc.hInstance);
    
    progressWindow = nullptr;
    progressBar = nullptr;
    statusText = nullptr;
    
    if (success)
    {
        MessageBoxA((HWND)parent, 
                   "DIE databases downloaded successfully!\nFile detection is now available.",
                   "Download Complete",
                   MB_OK | MB_ICONINFORMATION);
    }
    else
    {
        MessageBoxA((HWND)parent,
                   "Failed to download DIE databases.\nFile detection will be limited.",
                   "Download Failed",
                   MB_OK | MB_ICONWARNING);
    }
    
    return success;
}

#elif defined(__APPLE__)

void DIEDownloadDialog::ProgressCallback(const char* message, int percent)
{
    printf("%s (%d%%)\n", message, percent);
}

bool DIEDownloadDialog::Show(NativeWindow parent, bool darkMode)
{
    @autoreleasepool
    {
        NSAlert* alert = [[NSAlert alloc] init];
        [alert setMessageText:@"Download DIE Databases"];
        [alert setInformativeText:@"Downloading file detection databases. This may take a moment..."];
        [alert setAlertStyle:NSAlertStyleInformational];
        
        
        bool success = DownloadDIEDatabases(ProgressCallback);
        
        if (success)
        {
            NSAlert* successAlert = [[NSAlert alloc] init];
            [successAlert setMessageText:@"Download Complete"];
            [successAlert setInformativeText:@"DIE databases downloaded successfully!"];
            [successAlert setAlertStyle:NSAlertStyleInformational];
            [successAlert runModal];
        }
        else
        {
            NSAlert* failAlert = [[NSAlert alloc] init];
            [failAlert setMessageText:@"Download Failed"];
            [failAlert setInformativeText:@"Failed to download DIE databases."];
            [failAlert setAlertStyle:NSAlertStyleWarning];
            [failAlert runModal];
        }
        
        return success;
    }
}

#else

void DIEDownloadDialog::ProgressCallback(const char* message, int percent)
{
  printf("%s (%d%%)\n", message, percent);
}

bool DIEDownloadDialog::Show(void* parent, bool darkMode)
{
  printf("Downloading DIE databases...\n");

  bool success = DownloadDIEDatabases(ProgressCallback);

  if (success)
  {
    system("zenity --info --text='DIE databases downloaded successfully!' 2>/dev/null");
  }
  else
  {
    system("zenity --warning --text='Failed to download DIE databases.' 2>/dev/null");
  }

  return success;
}

#endif

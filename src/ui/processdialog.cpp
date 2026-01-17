#include "processdialog.h"

#ifdef _WIN32
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <dwmapi.h>
#endif

#ifdef __linux__
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#endif

#ifdef __APPLE__
#include <libproc.h>
#include <sys/types.h>
#endif

static ProcessDialogData* g_processDialogData = nullptr;

#ifdef _WIN32

bool EnumerateProcesses(ProcessList* list)
{
  if (!list || !list->entries)
    return false;

  HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snap == INVALID_HANDLE_VALUE)
    return false;

  PROCESSENTRY32 pe;
  memset(&pe, 0, sizeof(pe));
  pe.dwSize = sizeof(pe);

  if (!Process32First(snap, &pe))
  {
    CloseHandle(snap);
    return false;
  }

  list->count = 0;

  do
  {
    if (list->count >= list->capacity)
      break;

    if (pe.th32ProcessID == 0 || pe.th32ProcessID == 4)
      continue;

    if (pe.szExeFile[0] == '[')
      continue;

    ProcessEntry* e = &list->entries[list->count];
    e->pid = (int)pe.th32ProcessID;

    int i = 0;
    while (i < 259 && pe.szExeFile[i] != 0)
    {
      e->name[i] = (char)pe.szExeFile[i];
      i++;
    }
    e->name[i] = 0;

    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, (DWORD)e->pid);
    if (!h)
    {
      h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)e->pid);
    }

    if (h)
    {
      BOOL wow64 = FALSE;
      if (IsWow64Process(h, &wow64))
      {
        SYSTEM_INFO si;
        GetNativeSystemInfo(&si);

        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
        {
          e->is64bit = !wow64;
        }
        else
        {
          e->is64bit = false;
        }
      }
      else
      {
        e->is64bit = false;
      }

      CloseHandle(h);
    }
    else
    {
      e->is64bit = false;
    }

    list->count++;

  } while (Process32Next(snap, &pe));

  CloseHandle(snap);
  return true;
}

#endif

void RenderProcessDialog(ProcessDialogData* data, int windowWidth, int windowHeight)
{
  if (!data || !data->renderer)
    return;

  Theme theme = data->darkMode ? Theme::Dark() : Theme::Light();
  data->renderer->clear(theme.windowBackground);

  const int margin = 20;
  const int headerY = margin;
  const int rowHeight = 32;

  Color headerColor = theme.headerColor;
  data->renderer->drawText("PID", margin, headerY, headerColor);
  data->renderer->drawText("Process Name", margin + 80, headerY, headerColor);
  data->renderer->drawText("Architecture", margin + 320, headerY, headerColor);

  data->renderer->drawLine(margin, headerY + 20, windowWidth - margin, headerY + 20, theme.separator);

  int listStartY = headerY + 28;
  int listEndY = windowHeight - 70;
  int maxVisibleRows = (listEndY - listStartY) / rowHeight;

  int maxScroll = data->processes.count - maxVisibleRows;
  if (maxScroll < 0) maxScroll = 0;
  if (data->scrollOffset > maxScroll)
    data->scrollOffset = maxScroll;
  if (data->scrollOffset < 0)
    data->scrollOffset = 0;

  int rowY = listStartY;
  int visibleCount = 0;

  for (int i = data->scrollOffset; i < data->processes.count && visibleCount < maxVisibleRows; i++, visibleCount++)
  {
    ProcessEntry* e = &data->processes.entries[i];

    Rect rowRect(margin, rowY, windowWidth - margin * 2, rowHeight - 2);

    if (i == data->selectedIndex)
    {
      Color selectedBg = theme.controlCheck;
      selectedBg.a = 60;
      data->renderer->drawRoundedRect(rowRect, 6.0f, selectedBg, true);

      Color selectedBorder = theme.controlCheck;
      selectedBorder.a = 200;
      data->renderer->drawRoundedRect(rowRect, 6.0f, selectedBorder, false);
    }
    else if (i == data->hoveredIndex)
    {
      Color hoverBg = theme.menuHover;
      hoverBg.a = 80;
      data->renderer->drawRoundedRect(rowRect, 6.0f, hoverBg, true);
    }

    char pidBuf[32];
    IntToStr(e->pid, pidBuf, sizeof(pidBuf));
    Color textColor = (i == data->selectedIndex) ? theme.controlCheck : theme.textColor;
    data->renderer->drawText(pidBuf, margin, rowY + 8, textColor);

    data->renderer->drawText(e->name, margin + 80, rowY + 8, textColor);

    Rect archBadge(margin + 320, rowY + 6, 50, 20);
    bool isDark = (theme.windowBackground.r < 128);
    Color badgeBg = e->is64bit
      ? (isDark ? Color(50, 120, 200) : Color(70, 140, 220))
      : (isDark ? Color(180, 100, 50) : Color(220, 140, 80));
    badgeBg.a = (i == data->selectedIndex) ? 220 : 140;
    data->renderer->drawRoundedRect(archBadge, 4.0f, badgeBg, true);

    Color badgeText = Color(255, 255, 255);
    const char* archText = e->is64bit ? "x64" : "x86";
    data->renderer->drawText(archText, margin + 328, rowY + 9, badgeText);

    rowY += rowHeight;
  }

  if (data->processes.count > maxVisibleRows)
  {
    int scrollbarX = windowWidth - 15;
    int scrollbarY = listStartY;
    int scrollbarHeight = listEndY - listStartY;

    Rect trackRect(scrollbarX, scrollbarY, 8, scrollbarHeight);
    Color trackColor = theme.scrollbarBg;
    data->renderer->drawRoundedRect(trackRect, 4.0f, trackColor, true);

    float thumbRatio = (float)maxVisibleRows / (float)data->processes.count;
    int thumbHeight = (int)(scrollbarHeight * thumbRatio);
    if (thumbHeight < 30) thumbHeight = 30;

    float scrollRatio = (float)data->scrollOffset / (float)maxScroll;
    int thumbY = scrollbarY + (int)((scrollbarHeight - thumbHeight) * scrollRatio);

    Rect thumbRect(scrollbarX, thumbY, 8, thumbHeight);
    Color thumbColor = theme.scrollbarThumb;
    thumbColor.a = 200;
    data->renderer->drawRoundedRect(thumbRect, 4.0f, thumbColor, true);
  }

  data->renderer->drawLine(margin, listEndY + 8, windowWidth - margin, listEndY + 8, theme.separator);
 

  const int buttonWidth = 100;
  const int buttonHeight = 36;
  const int buttonY = windowHeight - margin - buttonHeight;

  {
    Rect r(windowWidth - margin - buttonWidth * 2 - 10, buttonY, buttonWidth, buttonHeight);
    WidgetState ws(r);
    ws.hovered = (data->hoveredWidget == 1);
    ws.pressed = (data->pressedWidget == 1);
    data->renderer->drawModernButton(ws, theme, "Cancel");
  }

  {
    Rect r(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
    WidgetState ws(r);
    ws.enabled = (data->selectedIndex >= 0);
    ws.hovered = (data->hoveredWidget == 2);
    ws.pressed = (data->pressedWidget == 2);
    data->renderer->drawModernButton(ws, theme, "Open");
  }
}

void UpdateProcessDialogHover(ProcessDialogData* data, int x, int y, int windowWidth, int windowHeight)
{
  const int margin = 20;
  const int headerY = margin;
  const int rowHeight = 32;
  const int listStartY = headerY + 28;
  const int listEndY = windowHeight - 70;

  data->hoveredIndex = -1;
  data->hoveredWidget = -1;

  const int buttonWidth = 100;
  const int buttonHeight = 36;
  const int buttonY = windowHeight - margin - buttonHeight;

  Rect cancelRect(windowWidth - margin - buttonWidth * 2 - 10, buttonY, buttonWidth, buttonHeight);
  if (x >= cancelRect.x && x <= cancelRect.x + cancelRect.width &&
    y >= cancelRect.y && y <= cancelRect.y + cancelRect.height)
  {
    data->hoveredWidget = 1;
    return;
  }

  Rect openRect(windowWidth - margin - buttonWidth, buttonY, buttonWidth, buttonHeight);
  if (x >= openRect.x && x <= openRect.x + openRect.width &&
    y >= openRect.y && y <= openRect.y + openRect.height)
  {
    data->hoveredWidget = 2;
    return;
  }

  if (y < listStartY || y > listEndY)
    return;

  int relY = y - listStartY;
  int rowIndex = relY / rowHeight;
  int absoluteIndex = data->scrollOffset + rowIndex;

  if (absoluteIndex >= 0 && absoluteIndex < data->processes.count)
  {
    data->hoveredIndex = absoluteIndex;
  }
}

void HandleProcessDialogClick(ProcessDialogData* data)
{
  if (data->hoveredWidget == 1)
  {
    data->dialogResult = false;
    data->running = false;
  }
  else if (data->hoveredWidget == 2 && data->selectedIndex >= 0)
  {
    data->dialogResult = true;
    data->running = false;
  }
  else if (data->hoveredIndex >= 0)
  {
    data->selectedIndex = data->hoveredIndex;
  }
}

#ifdef _WIN32

LRESULT CALLBACK ProcessDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  ProcessDialogData* data = g_processDialogData;

  switch (msg)
  {
  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    if (data && data->renderer)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);

      data->renderer->beginFrame();
      RenderProcessDialog(data, rect.right, rect.bottom);
      data->renderer->endFrame(hdc);
    }

    EndPaint(hwnd, &ps);
    return 0;
  }

  case WM_MOUSEMOVE:
  {
    if (data)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      UpdateProcessDialogHover(data, x, y, rect.right, rect.bottom);
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_MOUSEWHEEL:
  {
    if (data)
    {
      int delta = GET_WHEEL_DELTA_WPARAM(wParam);

      RECT rect;
      GetClientRect(hwnd, &rect);
      int maxVisibleRows = (rect.bottom - 118) / 32;
      int maxScroll = data->processes.count - maxVisibleRows;
      if (maxScroll < 0) maxScroll = 0;

      int scrollLines = (delta > 0) ? -3 : 3;
      data->scrollOffset += scrollLines;

      if (data->scrollOffset < 0)
        data->scrollOffset = 0;
      if (data->scrollOffset > maxScroll)
        data->scrollOffset = maxScroll;

      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONDOWN:
  {
    if (data)
    {
      data->pressedWidget = data->hoveredWidget;
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_LBUTTONUP:
  {
    if (data)
    {
      if (data->pressedWidget == data->hoveredWidget || data->hoveredIndex >= 0)
      {
        HandleProcessDialogClick(data);
      }
      data->pressedWidget = -1;
      InvalidateRect(hwnd, NULL, FALSE);
    }
    return 0;
  }

  case WM_CLOSE:
    if (data)
    {
      data->dialogResult = false;
      data->running = false;
    }
    return 0;

  case WM_SIZE:
  {
    if (data && data->renderer)
    {
      RECT rect;
      GetClientRect(hwnd, &rect);
      if (rect.right > 0 && rect.bottom > 0)
      {
        data->renderer->resize(rect.right, rect.bottom);
        InvalidateRect(hwnd, NULL, FALSE);
      }
    }
    return 0;
  }
  }

  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool ShowProcessDialog(HWND parent, AppOptions& options)
{
  const wchar_t* className = L"ProcessDialogWindow";
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(WNDCLASSEXW);
  wc.lpfnWndProc = ProcessDialogProc;
  wc.hInstance = GetModuleHandleW(NULL);
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

  UnregisterClassW(className, wc.hInstance);
  if (!RegisterClassExW(&wc))
    return false;

  ProcessDialogData data = {};
  data.darkMode = options.darkMode;
  data.dialogResult = false;
  data.running = true;
  data.selectedIndex = -1;
  data.hoveredIndex = -1;
  data.hoveredWidget = -1;
  data.pressedWidget = -1;
  data.scrollOffset = 0;

  g_processDialogData = &data;

  data.processes.capacity = 512;
  data.processes.count = 0;
  data.processes.entries = (ProcessEntry*)PlatformAlloc(sizeof(ProcessEntry) * 512);

  if (!data.processes.entries)
  {
    g_processDialogData = nullptr;
    return false;
  }

  for (int i = 0; i < 512; i++)
  {
    data.processes.entries[i].pid = 0;
    data.processes.entries[i].name[0] = '\0';
    data.processes.entries[i].is64bit = false;
  }

  if (!EnumerateProcesses(&data.processes))
  {
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  int width = 500;
  int height = 600;

  RECT parentRect;
  GetWindowRect(parent, &parentRect);
  int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
  int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;

  HWND hwnd = CreateWindowExW(
    WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
    className,
    L"Select Process",
    WS_POPUP | WS_CAPTION | WS_SYSMENU,
    x, y, width, height,
    parent,
    nullptr,
    GetModuleHandleW(NULL),
    nullptr);

  if (!hwnd)
  {
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  data.window = hwnd;

  if (data.darkMode)
  {
    BOOL dark = TRUE;
    constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  }

  data.renderer = new RenderManager();
  if (!data.renderer || !data.renderer->initialize(hwnd))
  {
    if (data.renderer)
      delete data.renderer;
    DestroyWindow(hwnd);
    PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
    g_processDialogData = nullptr;
    return false;
  }

  RECT clientRect;
  GetClientRect(hwnd, &clientRect);
  if (clientRect.right > 0 && clientRect.bottom > 0)
  {
    data.renderer->resize(clientRect.right, clientRect.bottom);
  }

  EnableWindow(parent, FALSE);
  ShowWindow(hwnd, SW_SHOW);
  UpdateWindow(hwnd);

  MSG msg;
  while (data.running && GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  int selectedPid = -1;
  if (data.dialogResult && data.selectedIndex >= 0 && data.selectedIndex < data.processes.count)
  {
    selectedPid = data.processes.entries[data.selectedIndex].pid;
  }

  if (data.renderer)
  {
    delete data.renderer;
    data.renderer = nullptr;
  }

  EnableWindow(parent, TRUE);
  SetForegroundWindow(parent);
  DestroyWindow(hwnd);
  UnregisterClassW(className, GetModuleHandleW(NULL));

  PlatformFree(data.processes.entries, sizeof(ProcessEntry) * 512);
  g_processDialogData = nullptr;

  if (selectedPid > 0)
  {
    char msg[256];
    StrCopy(msg, "Selected PID: ");
    char pidBuf[32];
    IntToStr(selectedPid, pidBuf, sizeof(pidBuf));
    StrCat(msg, pidBuf);
    MessageBoxA(parent, msg, "Process Selected", MB_OK);
  }

  return data.dialogResult;
}

#endif

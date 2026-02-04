#pragma once
#include "global.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__APPLE__)
#include <Cocoa/Cocoa.h>
#else
#include <X11/Xlib.h>
#endif

class DIEDownloadDialog
{
public:
#if defined(_WIN32)
  static bool Show(HWND parent, bool darkMode);
#elif defined(__APPLE__)
  static bool Show(NSWindow* parent, bool darkMode);
#else
  static bool Show(void* parent, bool darkMode);
#endif

private:
  static void ProgressCallback(const char* message, int percent);

#if defined(_WIN32)
  static HWND progressWindow;
  static HWND progressBar;
  static HWND statusText;
#endif
};

#pragma once
#include "global.h"

#if defined(_WIN32)
#include <windows.h>
typedef HWND NativeWindow;
#elif defined(__APPLE__)
#include <Cocoa/Cocoa.h>
typedef NSWindow* NativeWindow;
#else
#include <X11/Xlib.h>
typedef void* NativeWindow;
#endif

class DIEDownloadDialog
{
public:
  static bool Show(NativeWindow parent, bool darkMode);

private:
  static void ProgressCallback(const char* message, int percent);

#if defined(_WIN32)
  static HWND progressWindow;
  static HWND progressBar;
  static HWND statusText;
#endif
};
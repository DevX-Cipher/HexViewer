#pragma once
#include "render.h"
#include <functional>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <dwmapi.h>
#elif defined(__APPLE__)
#include <objc/objc.h>
typedef void* id;
#elif defined(__linux__)
#include <X11/Xlib.h>
#endif

namespace SearchDialogs {

  void CleanupDialogs() {};

  void ShowFindReplaceDialog(void* parentHandle, bool darkMode,
    std::function<void(const std::string&, const std::string&)> callback);

  void ShowGoToDialog(void* parentHandle, bool darkMode,
    std::function<void(int)> callback);

} // namespace SearchDialogs
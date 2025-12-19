#include "update.h"
#include <sstream>
#include <algorithm>
#include <thread>
#include <chrono>
#include <fstream>

#ifdef _WIN32
#include <shellapi.h>
#include <wininet.h>
#include <shlobj.h>
#include <appmodel.h>
#elif __APPLE__
#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h>
#include <sys/stat.h>
#elif __linux__
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#endif

#include <language.h>
#include <functional>

RenderManager* UpdateDialog::renderer = nullptr;
UpdateInfo UpdateDialog::currentInfo = {};
bool UpdateDialog::darkMode = true;
NativeWindow UpdateDialog::parentWindow = 0;
int UpdateDialog::hoveredButton = 0;
int UpdateDialog::pressedButton = 0;
int UpdateDialog::scrollOffset = 0;
bool UpdateDialog::scrollbarHovered = false;
bool UpdateDialog::scrollbarPressed = false;
int UpdateDialog::scrollDragStart = 0;
int UpdateDialog::scrollOffsetStart = 0;
Rect UpdateDialog::updateButtonRect = {};
Rect UpdateDialog::skipButtonRect = {};
Rect UpdateDialog::closeButtonRect = {};
Rect UpdateDialog::scrollbarRect = {};
Rect UpdateDialog::scrollThumbRect = {};

static DownloadState downloadState = DownloadState::Idle;
static float downloadProgress = 0.0f;
static std::string downloadStatus = "";
static std::string downloadedFilePath = "";
static Rect progressBarRect = {};
static std::thread* downloadThread = nullptr;
static bool betaEnabled = false;
static Rect betaToggleRect = {};
static bool betaToggleHovered = false;

#ifdef __linux__
static Display* display = nullptr;
static Window window = 0;
static Atom wmDeleteWindow;
#elif __APPLE__
static NSWindow* nsWindow = nullptr;
static bool windowShouldClose = false;
#endif
#ifdef _WIN32
extern bool g_isMsix;
#endif
extern bool g_isNative;

static bool IsMsixPackage() {
#ifdef _WIN32
  return g_isMsix;
#else
  return false;
#endif
}
std::string GetTempDownloadPath() {
#ifdef _WIN32
  wchar_t tempPath[MAX_PATH];
  GetTempPathW(MAX_PATH, tempPath);
  char temp[MAX_PATH];
  WideCharToMultiByte(CP_UTF8, 0, tempPath, -1, temp, MAX_PATH, nullptr, nullptr);
  return std::string(temp) + "update_download";
#else
  return "/tmp/update_download";
#endif
}

std::string ExtractJsonValue(const std::string& json, const std::string& key) {
  std::string searchKey = "\"" + key + "\"";
  size_t pos = json.find(searchKey);
  if (pos == std::string::npos) return "";

  pos = json.find(":", pos);
  if (pos == std::string::npos) return "";

  pos = json.find("\"", pos);
  if (pos == std::string::npos) return "";

  size_t endPos = json.find("\"", pos + 1);
  if (endPos == std::string::npos) return "";

  return json.substr(pos + 1, endPos - pos - 1);
}

bool ExtractJsonBool(const std::string& json, const std::string& key) {
  std::string searchKey = "\"" + key + "\"";
  size_t pos = json.find(searchKey);
  if (pos == std::string::npos) return false;

  pos = json.find(":", pos);
  if (pos == std::string::npos) return false;

  size_t truePos = json.find("true", pos);
  size_t falsePos = json.find("false", pos);
  size_t commaPos = json.find(",", pos);
  size_t bracePos = json.find("}", pos);

  size_t endPos = std::clamp(commaPos, size_t{ 0 }, bracePos);

  if (truePos != std::string::npos && truePos < endPos) {
    return true;
  }
  return false;
}

std::string HttpGet(const std::string& url) {
#ifdef _WIN32
  HINTERNET hInternet = InternetOpenA("UpdaterAgent/1.0",
    INTERNET_OPEN_TYPE_PRECONFIG,
    nullptr, nullptr, 0);
  if (!hInternet) return "";

  DWORD timeout = 5000; // 5 seconds
  InternetSetOptionA(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
  InternetSetOptionA(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

  HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0,
    INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE, 0);
  if (!hConnect) {
    InternetCloseHandle(hInternet);
    return "";
  }

  std::string result;
  char buffer[4096];
  DWORD bytesRead;

  while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
    result.append(buffer, bytesRead);
  }

  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);
  return result;
#else
  std::string command = "curl --max-time 5 -L -s \"" + url + "\"";
  FILE* pipe = popen(command.c_str(), "r");
  if (!pipe) return "";

  std::string result;
  char buffer[4096];
  while (fgets(buffer, sizeof(buffer), pipe)) {
    result += buffer;
  }
  pclose(pipe);
  return result;
#endif
}

bool DownloadFile(const std::string& url, const std::string& outputPath,
  std::function<void(float, const std::string&)> progressCallback) {
#ifdef _WIN32
  HINTERNET hInternet = InternetOpenA("UpdaterAgent/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (!hInternet) return false;

  HINTERNET hConnect = InternetOpenUrlA(hInternet, url.c_str(), nullptr, 0,
    INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_SECURE, 0);
  if (!hConnect) {
    InternetCloseHandle(hInternet);
    return false;
  }

  DWORD fileSize = 0;
  DWORD bufferSize = sizeof(fileSize);
  HttpQueryInfoA(hConnect, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER,
    &fileSize, &bufferSize, nullptr);

  std::ofstream outFile(outputPath, std::ios::binary);
  if (!outFile) {
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);
    return false;
  }

  char buffer[8192];
  DWORD bytesRead;
  DWORD totalRead = 0;

  while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
    outFile.write(buffer, bytesRead);
    totalRead += bytesRead;

    if (fileSize > 0) {
      float progress = (float)totalRead / (float)fileSize;
      std::ostringstream status;
      status << "Downloaded " << (totalRead / 1024) << " KB / " << (fileSize / 1024) << " KB";
      progressCallback(progress, status.str());
    }
  }

  outFile.close();
  InternetCloseHandle(hConnect);
  InternetCloseHandle(hInternet);
  return true;
#else
  std::string command = "curl -L \"" + url + "\" -o \"" + outputPath + "\"";
  int result = system(command.c_str());
  progressCallback(1.0f, "Download complete");
  return result == 0;
#endif
}

std::string GetAssetDownloadUrl(const std::string& releaseApiUrl, bool includeBeta) {
  bool isMsix = IsMsixPackage();

  if (isMsix) {
    return "";
  }

  std::string modifiedUrl = releaseApiUrl;
  if (includeBeta) {
    size_t pos = modifiedUrl.find("/releases/latest");
    if (pos != std::string::npos) {
      modifiedUrl = modifiedUrl.substr(0, pos) + "/releases";
    }
  }

  std::string response = HttpGet(modifiedUrl);
  if (response.empty()) {
    return "";
  }

  bool isNative = g_isNative;
  
  std::vector<std::string> targetExtensions;
  
#ifdef _WIN32
  if (isNative) {
    targetExtensions = {".exe", ".msi", "-windows.exe", "-win.exe", "-win64.exe"};
  } else {
    targetExtensions = {".zip", "-windows.zip", "-win.zip", "-win64.zip"};
  }
#elif __APPLE__
  if (isNative) {
    targetExtensions = {".dmg", "-macos.dmg", "-mac.dmg", "-osx.dmg"};
  } else {
    targetExtensions = {".zip", "-macos.zip", "-mac.zip", "-osx.zip"};
  }
#elif __linux__
  if (isNative) {
    targetExtensions = {".deb", ".AppImage", "-linux.deb", "-linux.AppImage", 
                        ".rpm", "-x86_64.deb", "-amd64.deb"};
  } else {
    targetExtensions = {".tar.gz", ".tar.bz2", "-linux.tar.gz", "-linux.tar.bz2"};
  }
#else
  targetExtensions = {".zip"};
#endif

  std::string releaseToSearch = response;

  if (includeBeta && response.length() > 2 && response[0] == '[') {
    size_t searchPos = 0;
    std::string firstRelease;
    std::string firstPrerelease;
    
    while (searchPos < response.length()) {
      size_t releaseStart = response.find("{\"url\"", searchPos);
      if (releaseStart == std::string::npos) {
        releaseStart = response.find("{\"id\"", searchPos);
      }
      if (releaseStart == std::string::npos) break;
      
      int braceCount = 0;
      size_t releaseEnd = releaseStart;
      bool inString = false;
      bool escaped = false;
      
      for (size_t i = releaseStart; i < response.length(); i++) {
        char c = response[i];
        
        if (escaped) {
          escaped = false;
          continue;
        }
        
        if (c == '\\') {
          escaped = true;
          continue;
        }
        
        if (c == '"') {
          inString = !inString;
        }
        
        if (!inString) {
          if (c == '{') braceCount++;
          if (c == '}') {
            braceCount--;
            if (braceCount == 0) {
              releaseEnd = i;
              break;
            }
          }
        }
      }
      
      if (releaseEnd <= releaseStart) break;
      
      std::string currentRelease = response.substr(releaseStart, releaseEnd - releaseStart + 1);
      
      if (firstRelease.empty()) {
        firstRelease = currentRelease;
      }
      
      bool isPrerelease = ExtractJsonBool(currentRelease, "prerelease");
      
      if (isPrerelease && firstPrerelease.empty()) {
        firstPrerelease = currentRelease;
      }
      
      searchPos = releaseEnd + 1;
    }
    
    releaseToSearch = !firstPrerelease.empty() ? firstPrerelease : firstRelease;
    
    if (releaseToSearch.empty()) {
      releaseToSearch = response; // Fallback to full response
    }
  }

  size_t assetsPos = releaseToSearch.find("\"assets\"");
  if (assetsPos == std::string::npos) {
    return "";
  }

  size_t assetsArrayStart = releaseToSearch.find("[", assetsPos);
  if (assetsArrayStart == std::string::npos) {
    return "";
  }

  size_t assetsArrayEnd = releaseToSearch.find("]", assetsArrayStart);
  if (assetsArrayEnd == std::string::npos) {
    assetsArrayEnd = releaseToSearch.length();
  }

  std::string assetsSection = releaseToSearch.substr(assetsArrayStart, 
                                                      assetsArrayEnd - assetsArrayStart);

  std::vector<std::pair<std::string, std::string>> availableAssets;
  
  size_t currentPos = 0;
  while (currentPos < assetsSection.length()) {
    size_t namePos = assetsSection.find("\"name\"", currentPos);
    if (namePos == std::string::npos) break;

    std::string assetName = ExtractJsonValue(assetsSection.substr(namePos), "name");
    
    if (!assetName.empty()) {
      size_t urlPos = assetsSection.find("\"browser_download_url\"", namePos);
      if (urlPos != std::string::npos) {
        std::string downloadUrl = ExtractJsonValue(assetsSection.substr(urlPos), 
                                                    "browser_download_url");
        if (!downloadUrl.empty()) {
          availableAssets.push_back({assetName, downloadUrl});
        }
      }
    }
    
    currentPos = namePos + 1;
  }

  for (const auto& extension : targetExtensions) {
    for (const auto& asset : availableAssets) {
      const std::string& assetName = asset.first;
      
      if (assetName.length() >= extension.length()) {
        std::string assetEnd = assetName.substr(assetName.length() - extension.length());
        
        std::string assetEndLower = assetEnd;
        std::string extensionLower = extension;
        std::transform(assetEndLower.begin(), assetEndLower.end(), 
                      assetEndLower.begin(), ::tolower);
        std::transform(extensionLower.begin(), extensionLower.end(), 
                      extensionLower.begin(), ::tolower);
        
        if (assetEndLower == extensionLower) {
          return asset.second;
        }
      }
    }
  }

  for (const auto& extension : targetExtensions) {
    for (const auto& asset : availableAssets) {
      std::string assetNameLower = asset.first;
      std::string extensionLower = extension;
      std::transform(assetNameLower.begin(), assetNameLower.end(), 
                    assetNameLower.begin(), ::tolower);
      std::transform(extensionLower.begin(), extensionLower.end(), 
                    extensionLower.begin(), ::tolower);
      
      if (assetNameLower.find(extensionLower) != std::string::npos) {
        return asset.second;
      }
    }
  }

  if (!availableAssets.empty()) {
    return availableAssets[0].second;
  }

  return "";
}

void DownloadFromGitHub() {
  bool isMsix = IsMsixPackage();

  if (isMsix) {
    downloadState = DownloadState::Error;
    downloadStatus = "MSIX packages update through Microsoft Store";
    return;
  }

  downloadState = DownloadState::Connecting;
  downloadStatus = "Connecting to GitHub...";
  downloadProgress = 0.0f;

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::string assetUrl = GetAssetDownloadUrl(UpdateDialog::currentInfo.releaseApiUrl, betaEnabled);

  if (assetUrl.empty()) {
    downloadState = DownloadState::Error;
    downloadStatus = "No matching asset found for your platform";
    return;
  }

  downloadState = DownloadState::Downloading;
  downloadStatus = betaEnabled ? "Downloading beta update..." : "Downloading update...";
  downloadProgress = 0.05f;

  std::string filename = "update_download";
  size_t lastSlash = assetUrl.find_last_of('/');
  if (lastSlash != std::string::npos && lastSlash + 1 < assetUrl.length()) {
    filename = assetUrl.substr(lastSlash + 1);

    size_t questionMark = filename.find('?');
    if (questionMark != std::string::npos) {
      filename = filename.substr(0, questionMark);
    }
  }

#ifdef _WIN32
  wchar_t tempPath[MAX_PATH];
  GetTempPathW(MAX_PATH, tempPath);
  char temp[MAX_PATH];
  WideCharToMultiByte(CP_UTF8, 0, tempPath, -1, temp, MAX_PATH, nullptr, nullptr);
  downloadedFilePath = std::string(temp) + filename;
#else
  downloadedFilePath = "/tmp/" + filename;
#endif

  bool success = DownloadFile(assetUrl, downloadedFilePath,
    [](float progress, const std::string& status) {
      downloadProgress = progress;
      downloadStatus = status;
    });

  if (!success) {
    downloadState = DownloadState::Error;
    downloadStatus = "Download failed. Please check your internet connection.";
    return;
  }

  std::ifstream testFile(downloadedFilePath, std::ios::binary);
  if (!testFile.good()) {
    downloadState = DownloadState::Error;
    downloadStatus = "Downloaded file verification failed";
    return;
  }
  testFile.close();

  downloadState = DownloadState::Complete;
  downloadStatus = "Download complete! Click Install to proceed.";
  downloadProgress = 1.0f;
}

void InstallUpdate() {
  if (downloadedFilePath.empty()) return;

  std::string extension;
  size_t dotPos = downloadedFilePath.find_last_of('.');
  if (dotPos != std::string::npos) {
    extension = downloadedFilePath.substr(dotPos);
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);
  }

#ifdef _WIN32
  if (extension == ".exe" || extension == ".msi") {
    std::wstring wPath(downloadedFilePath.begin(), downloadedFilePath.end());
    SHELLEXECUTEINFOW sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"open";
    sei.lpFile = wPath.c_str();
    sei.nShow = SW_SHOWNORMAL;

    if (ShellExecuteExW(&sei)) {
      PostQuitMessage(0);

      if (UpdateDialog::parentWindow) {
        PostMessage((HWND)UpdateDialog::parentWindow, WM_CLOSE, 0, 0);
      }
    }
    else {
      downloadState = DownloadState::Error;
      downloadStatus = "Failed to launch installer";
    }
  }
  else {
    std::wstring wPath(downloadedFilePath.begin(), downloadedFilePath.end());
    ShellExecuteW(nullptr, L"open", L"explorer.exe",
      (L"/select,\"" + wPath + L"\"").c_str(), nullptr, SW_SHOWNORMAL);
  }

#elif __APPLE__
  if (extension == ".dmg" || extension == ".pkg") {
    std::string cmd = "open \"" + downloadedFilePath + "\"";
    int result = system(cmd.c_str());

    if (result == 0) {
      @autoreleasepool{
        dispatch_async(dispatch_get_main_queue(),^ {
          [NSApp terminate : nil] ;
        });
      }
    }
    else {
      downloadState = DownloadState::Error;
      downloadStatus = "Failed to open installer";
    }
  }
  else {
    std::string cmd = "open -R \"" + downloadedFilePath + "\"";
    system(cmd.c_str());
  }

#elif __linux__
  if (extension == ".deb") {
    std::string gdebiCmd = "gdebi-gtk \"" + downloadedFilePath + "\" &";
    int result = system(gdebiCmd.c_str());

    if (result != 0) {
      std::string cmd = "x-terminal-emulator -e \"bash -c 'sudo dpkg -i \\\"" +
        downloadedFilePath + "\\\"; sudo apt-get install -f -y; " +
        "echo; echo Press Enter to close.; read'\" &";
      result = system(cmd.c_str());

      if (result != 0) {
        cmd = "gnome-terminal -- bash -c 'sudo dpkg -i \"" + downloadedFilePath +
          "\"; sudo apt-get install -f -y; echo; echo \"Press Enter to close.\"; read' &";
        result = system(cmd.c_str());

        if (result != 0) {
          cmd = "xterm -e 'sudo dpkg -i \"" + downloadedFilePath +
            "\"; sudo apt-get install -f -y; echo; echo Press Enter to close.; read' &";
          system(cmd.c_str());
        }
      }
    }

    exit(0);
  }
  else if (extension == ".appimage") {
    std::string chmodCmd = "chmod +x \"" + downloadedFilePath + "\"";
    system(chmodCmd.c_str());

    std::string cmd = "\"" + downloadedFilePath + "\" &";
    system(cmd.c_str());
    exit(0);
  }
  else if (extension == ".rpm") {
    std::string cmd = "x-terminal-emulator -e \"bash -c 'sudo rpm -i \\\"" +
      downloadedFilePath + "\\\"; echo; echo Press Enter to close.; read'\" &";
    int result = system(cmd.c_str());

    if (result != 0) {
      cmd = "gnome-terminal -- bash -c 'sudo rpm -i \"" + downloadedFilePath +
        "\"; echo; echo \"Press Enter to close.\"; read' &";
      system(cmd.c_str());
    }
    exit(0);
  }
  else {
    std::string dirPath = downloadedFilePath.substr(0, downloadedFilePath.find_last_of("/"));
    std::string cmd = "xdg-open \"" + dirPath + "\" &";
    system(cmd.c_str());
  }
#endif
}
void StartDownload() {
  if (downloadThread) {
    if (downloadThread->joinable()) {
      downloadThread->join();
    }
    delete downloadThread;
  }

  downloadThread = new std::thread([]() {
    DownloadFromGitHub();
    });
}

bool UpdateDialog::Show(NativeWindow parent, const UpdateInfo& info) {
  currentInfo = info;
  parentWindow = parent;
  hoveredButton = 0;
  pressedButton = 0;
  scrollOffset = 0;
  downloadState = DownloadState::Idle;
  downloadProgress = 0.0f;
  downloadStatus = "";
  betaEnabled = info.betaPreference;
  betaToggleHovered = false;

  int width = 600;
  int height = 500;

#ifdef _WIN32
  WNDCLASSEXW wcex = {};
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
  wcex.lpfnWndProc = DialogProc;
  wcex.hInstance = GetModuleHandle(nullptr);
  wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wcex.hbrBackground = nullptr;
  wcex.lpszClassName = L"UpdateDialogClass";

  RegisterClassExW(&wcex);

  HWND hWnd = CreateWindowExW(
    WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
    L"UpdateDialogClass",
    info.updateAvailable ? L"Update Available" : L"No Updates Available",
    WS_POPUP | WS_CAPTION | WS_SYSMENU,
    CW_USEDEFAULT, CW_USEDEFAULT,
    width, height,
    (HWND)parent,
    nullptr,
    GetModuleHandle(nullptr),
    nullptr
  );

  if (!hWnd) return false;

  if (parent) {
    RECT parentRect;
    GetWindowRect((HWND)parent, &parentRect);
    int x = parentRect.left + (parentRect.right - parentRect.left - width) / 2;
    int y = parentRect.top + (parentRect.bottom - parentRect.top - height) / 2;
    SetWindowPos(hWnd, nullptr, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
  }

  renderer = new RenderManager();
  if (!renderer->initialize(hWnd)) {
    DestroyWindow(hWnd);
    delete renderer;
    return false;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  renderer->resize(rc.right - rc.left, rc.bottom - rc.top);

  ShowWindow(hWnd, SW_SHOW);
  UpdateWindow(hWnd);

  MSG msg;
  bool result = false;
  while (GetMessage(&msg, nullptr, 0, 0)) {
    if (msg.message == WM_QUIT || !IsWindow(hWnd)) {
      break;
    }
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  if (downloadThread) {
    if (downloadThread->joinable()) {
      downloadThread->join();
    }
    delete downloadThread;
    downloadThread = nullptr;
  }

  delete renderer;
  renderer = nullptr;
  UnregisterClassW(L"UpdateDialogClass", GetModuleHandle(nullptr));

#elif __APPLE__
  @autoreleasepool{
    NSRect frame = NSMakeRect(0, 0, width, height);

    NSWindowStyleMask styleMask = NSWindowStyleMaskTitled |
                                  NSWindowStyleMaskClosable |
                                  NSWindowStyleMaskMiniaturizable;

    nsWindow = [[NSWindow alloc]initWithContentRect:frame
                                           styleMask : styleMask
                                             backing : NSBackingStoreBuffered
                                               defer : NO];

    NSString* title = info.updateAvailable ?
                      @"Update Available" : @"No Updates Available";
    [nsWindow setTitle:title] ;
    [nsWindow setLevel:NSFloatingWindowLevel] ;
    [nsWindow center] ;

    NSView* contentView = [[NSView alloc]initWithFrame:frame];
    [nsWindow setContentView:contentView] ;

    [nsWindow makeKeyAndOrderFront:nil] ;

    renderer = new RenderManager();
    if (!renderer->initialize((NativeWindow)nsWindow)) {
      [nsWindow close] ;
      nsWindow = nullptr;
      delete renderer;
      return false;
    }

    renderer->resize(width, height);

    bool running = true;
    windowShouldClose = false;

    while (running && !windowShouldClose) {
      @autoreleasepool {
        NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny
                                            untilDate : [NSDate distantPast]
                                               inMode : NSDefaultRunLoopMode
                                              dequeue : YES];

        if (event) {
          NSEventType eventType = [event type];

          switch (eventType) {
            case NSEventTypeLeftMouseDown: {
              NSPoint point = [event locationInWindow];
              OnMouseDown((int)point.x, (int)point.y);
              break;
            }

            case NSEventTypeLeftMouseUp: {
              NSPoint point = [event locationInWindow];
              if (OnMouseUp((int)point.x, (int)point.y)) {
                running = false;
              }
              break;
            }

            case NSEventTypeMouseMoved:
            case NSEventTypeLeftMouseDragged: {
              NSPoint point = [event locationInWindow];
              OnMouseMove((int)point.x, (int)point.y);
              break;
            }

            case NSEventTypeScrollWheel: {
              CGFloat deltaY = [event scrollingDeltaY];
              scrollOffset -= (int)(deltaY * 2);
              scrollOffset = std::clamp(scrollOffset, 0, INT_MAX);
              OnPaint();
              break;
            }

            default:
              [NSApp sendEvent:event] ;
              break;
          }
        }

        if (![nsWindow isVisible]) {
          running = false;
        }

        OnPaint();

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
      }
    }

    if (downloadThread) {
      if (downloadThread->joinable()) {
        downloadThread->join();
      }
      delete downloadThread;
      downloadThread = nullptr;
    }

    delete renderer;
    renderer = nullptr;
    [nsWindow close] ;
    nsWindow = nullptr;
  }

#elif __linux__
  display = XOpenDisplay(nullptr);
  if (!display) return false;

  int screen = DefaultScreen(display);
  Window root = RootWindow(display, screen);

  XSetWindowAttributes attrs = {};
  attrs.event_mask = ExposureMask | KeyPressMask | ButtonPressMask |
    ButtonReleaseMask | PointerMotionMask | StructureNotifyMask;
  attrs.background_pixel = WhitePixel(display, screen);

  window = XCreateWindow(
    display, root,
    0, 0, width, height, 0,
    CopyFromParent, InputOutput, CopyFromParent,
    CWBackPixel | CWEventMask,
    &attrs
    );

  const char* title = info.updateAvailable ? "Update Available" : "No Updates Available";
  XStoreName(display, window, title);
  XSetIconName(display, window, title);

  Atom wmWindowType = XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  Atom wmWindowTypeDialog = XInternAtom(display, "_NET_WM_WINDOW_TYPE_DIALOG", False);
  XChangeProperty(display, window, wmWindowType, XA_ATOM, 32, PropModeReplace,
    (unsigned char*)&wmWindowTypeDialog, 1);

  XSizeHints* sizeHints = XAllocSizeHints();
  sizeHints->flags = PMinSize | PMaxSize;
  sizeHints->min_width = sizeHints->max_width = width;
  sizeHints->min_height = sizeHints->max_height = height;
  XSetWMNormalHints(display, window, sizeHints);
  XFree(sizeHints);

  wmDeleteWindow = XInternAtom(display, "WM_DELETE_WINDOW", False);
  XSetWMProtocols(display, window, &wmDeleteWindow, 1);

  int screenWidth = DisplayWidth(display, screen);
  int screenHeight = DisplayHeight(display, screen);
  XMoveWindow(display, window, (screenWidth - width) / 2, (screenHeight - height) / 2);

  XMapWindow(display, window);
  XFlush(display);

  renderer = new RenderManager();
  if (!renderer->initialize((NativeWindow)window)) {
    XDestroyWindow(display, window);
    XCloseDisplay(display);
    delete renderer;
    return false;
  }

  renderer->resize(width, height);

  bool running = true;
  XEvent event;

  while (running) {
    while (XPending(display)) {
      XNextEvent(display, &event);

      switch (event.type) {
      case Expose:
        if (event.xexpose.count == 0) {
          OnPaint();
        }
        break;

      case ClientMessage:
        if ((Atom)event.xclient.data.l[0] == wmDeleteWindow) {
          running = false;
        }
        break;

      case MotionNotify:
        OnMouseMove(event.xmotion.x, event.xmotion.y);
        break;

      case ButtonPress:
        if (event.xbutton.button == Button1) {
          OnMouseDown(event.xbutton.x, event.xbutton.y);
        }
        else if (event.xbutton.button == Button4) { // Scroll up
          scrollOffset -= 20;
          scrollOffset = std::clamp(scrollOffset, 0, INT_MAX);
          OnPaint();
        }
        else if (event.xbutton.button == Button5) { // Scroll down
          scrollOffset += 20;
          OnPaint();
        }
        break;

      case ButtonRelease:
        if (event.xbutton.button == Button1) {
          if (OnMouseUp(event.xbutton.x, event.xbutton.y)) {
            running = false;
          }
        }
        break;
      }
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
  }

  if (downloadThread) {
    if (downloadThread->joinable()) {
      downloadThread->join();
    }
    delete downloadThread;
    downloadThread = nullptr;
  }

  delete renderer;
  renderer = nullptr;
  XDestroyWindow(display, window);
  XCloseDisplay(display);

#endif

  return true;
}

#ifdef _WIN32
LRESULT CALLBACK UpdateDialog::DialogProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
  switch (message) {
  case WM_CREATE:
    break;

  case WM_ERASEBKGND:
    return 1;

  case WM_PAINT:
    OnPaint(hWnd);
    return 0;

  case WM_MOUSEMOVE: {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseMove(hWnd, x, y);
    break;
  }

  case WM_LBUTTONDOWN: {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseDown(hWnd, x, y);
    break;
  }

  case WM_LBUTTONUP: {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    OnMouseUp(hWnd, x, y);
    break;
  }

  case WM_MOUSEWHEEL: {
    int delta = GET_WHEEL_DELTA_WPARAM(wParam);
    scrollOffset -= (delta > 0 ? 20 : -20);
    scrollOffset = std::clamp(scrollOffset, 0, scrollOffset);
    InvalidateRect(hWnd, nullptr, FALSE);
    break;
  }

  case WM_CLOSE:
    DestroyWindow(hWnd);
    return 0;

  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

void UpdateDialog::OnPaint(HWND hWnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hWnd, &ps);

  if (!renderer) {
    EndPaint(hWnd, &ps);
    return;
  }

  RECT rc;
  GetClientRect(hWnd, &rc);
  int width = rc.right - rc.left;
  int height = rc.bottom - rc.top;

  RenderContent(width, height);

  if (downloadState != DownloadState::Idle && downloadState != DownloadState::Complete && downloadState != DownloadState::Error) {
    InvalidateRect(hWnd, nullptr, FALSE);
  }

  EndPaint(hWnd, &ps);
}

void UpdateDialog::OnMouseMove(HWND hWnd, int x, int y) {
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;
  bool isMsix = IsMsixPackage();

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix) {
    if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
      y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height) {
      betaToggleHovered = true;
    }
  }

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix) {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
      y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height) {
      hoveredButton = 2;
    }
  }
  else if (downloadState == DownloadState::Complete && !isMsix) {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
      hoveredButton = 1;  // Install button
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
      y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height) {
      hoveredButton = 2;  // Cancel button
    }
  }
  else if ((downloadState == DownloadState::Idle && currentInfo.updateAvailable && isMsix) ||
    (downloadState == DownloadState::Idle && !currentInfo.updateAvailable) ||
    downloadState == DownloadState::Error) {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
      y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height) {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered || oldBetaHovered != betaToggleHovered) {
    InvalidateRect(hWnd, nullptr, FALSE);
  }
}

void UpdateDialog::OnMouseDown(HWND hWnd, int x, int y) {
  pressedButton = hoveredButton;

  if (betaToggleHovered) {
    betaEnabled = !betaEnabled;
    InvalidateRect(hWnd, nullptr, FALSE);
    return;
  }

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height) {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
    SetCapture(hWnd);
  }

  InvalidateRect(hWnd, nullptr, FALSE);
}

bool UpdateDialog::OnMouseUp(HWND hWnd, int x, int y) {
  if (scrollbarPressed) {
    scrollbarPressed = false;
    ReleaseCapture();
  }

  if (pressedButton == hoveredButton && pressedButton > 0) {
    if (pressedButton == 1 && downloadState == DownloadState::Idle) {
      StartDownload();
      InvalidateRect(hWnd, nullptr, FALSE);
      return false;
    }
    else if (pressedButton == 1 && downloadState == DownloadState::Complete) {
      InstallUpdate();
      return false;
    }
    else if (pressedButton == 2 || pressedButton == 3) {
      DestroyWindow(hWnd);
      return true;
    }
  }

  pressedButton = 0;
  InvalidateRect(hWnd, nullptr, FALSE);
  return false;
}
#elif __linux__

void UpdateDialog::OnPaint() {
  if (!renderer) return;

  RenderContent(600, 500);
  XFlush(display);
}

void UpdateDialog::OnMouseMove(int x, int y) {
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable) {
    if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
      y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height) {
      betaToggleHovered = true;
    }
  }

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable) {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
      y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height) {
      hoveredButton = 2;
    }
  }
  else if ((downloadState == DownloadState::Idle && currentInfo.updateAvailable) ||
    (downloadState == DownloadState::Idle && !currentInfo.updateAvailable) ||
    downloadState == DownloadState::Complete ||
    downloadState == DownloadState::Error) {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
      y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height) {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered || oldBetaHovered != betaToggleHovered) {
    OnPaint();
  }
}

void UpdateDialog::OnMouseDown(int x, int y) {
  pressedButton = hoveredButton;

  if (betaToggleHovered) {
    betaEnabled = !betaEnabled;
    OnPaint();
    return;
  }

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height) {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
  }

  OnPaint();
}

bool UpdateDialog::OnMouseUp(int x, int y) {
  if (scrollbarPressed) {
    scrollbarPressed = false;
  }

  if (pressedButton == hoveredButton && pressedButton > 0) {
    if (pressedButton == 1 && downloadState == DownloadState::Idle) {
      StartDownload();
      OnPaint();
      return false;
    }
    else if (pressedButton == 1 && downloadState == DownloadState::Complete) {
      InstallUpdate();
      return false;
    }
    else if (pressedButton == 2 || pressedButton == 3) {
      return true;
    }
  }

  pressedButton = 0;
  OnPaint();
  return false;
}

#elif __APPLE__

void UpdateDialog::OnPaint() {
  if (!renderer) return;

  RenderContent(600, 500);

  @autoreleasepool {
    [[nsWindow contentView]setNeedsDisplay:YES];
  }
}

void UpdateDialog::OnMouseMove(int x, int y) {
  int oldHovered = hoveredButton;
  bool oldBetaHovered = betaToggleHovered;
  hoveredButton = 0;
  betaToggleHovered = false;

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable) {
    if (x >= betaToggleRect.x && x <= betaToggleRect.x + betaToggleRect.width &&
      y >= betaToggleRect.y && y <= betaToggleRect.y + betaToggleRect.height) {
      betaToggleHovered = true;
    }
  }

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable) {
    if (x >= updateButtonRect.x && x <= updateButtonRect.x + updateButtonRect.width &&
      y >= updateButtonRect.y && y <= updateButtonRect.y + updateButtonRect.height) {
      hoveredButton = 1;
    }
    else if (x >= skipButtonRect.x && x <= skipButtonRect.x + skipButtonRect.width &&
      y >= skipButtonRect.y && y <= skipButtonRect.y + skipButtonRect.height) {
      hoveredButton = 2;
    }
  }
  else if (downloadState == DownloadState::Idle && !currentInfo.updateAvailable) {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
      y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height) {
      hoveredButton = 3;
    }
  }
  else if (downloadState == DownloadState::Complete || downloadState == DownloadState::Error) {
    if (x >= closeButtonRect.x && x <= closeButtonRect.x + closeButtonRect.width &&
      y >= closeButtonRect.y && y <= closeButtonRect.y + closeButtonRect.height) {
      hoveredButton = 3;
    }
  }

  bool wasScrollbarHovered = scrollbarHovered;
  scrollbarHovered = (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height);

  if (oldHovered != hoveredButton || wasScrollbarHovered != scrollbarHovered || oldBetaHovered != betaToggleHovered) {
    OnPaint();
  }
}

void UpdateDialog::OnMouseDown(int x, int y) {
  pressedButton = hoveredButton;

  if (betaToggleHovered) {
    betaEnabled = !betaEnabled;
    OnPaint();
    return;
  }

  if (x >= scrollThumbRect.x && x <= scrollThumbRect.x + scrollThumbRect.width &&
    y >= scrollThumbRect.y && y <= scrollThumbRect.y + scrollThumbRect.height) {
    scrollbarPressed = true;
    scrollDragStart = y;
    scrollOffsetStart = scrollOffset;
  }

  OnPaint();
}

bool UpdateDialog::OnMouseUp(int x, int y) {
  if (scrollbarPressed) {
    scrollbarPressed = false;
  }

  if (pressedButton == hoveredButton && pressedButton > 0) {
    if (pressedButton == 1 && downloadState == DownloadState::Idle) {
      StartDownload();
      OnPaint();
      return false;
    }
    else if (pressedButton == 1 && downloadState == DownloadState::Complete) {
      InstallUpdate();
      return false;
    }
    else if (pressedButton == 2 || pressedButton == 3) {
      return true;
    }
  }

  pressedButton = 0;
  OnPaint();
  return false;
}
#endif

void UpdateDialog::RenderContent(int width, int height) {
  if (!renderer) return;

  Theme theme = darkMode ? Theme::Dark() : Theme::Light();
  bool isMsix = IsMsixPackage();

  renderer->beginFrame();
  renderer->clear(theme.windowBackground);

  Rect headerRect(0, 0, width, 80);
  renderer->drawRect(headerRect, theme.menuBackground, true);

  Rect iconRect(20, 20, 40, 40);
  renderer->drawRect(iconRect, Color(100, 150, 255), true);
  renderer->drawText("i", 35, 32, Color::White());

  std::string title = currentInfo.updateAvailable ?
    Translations::T("Update Available!") : Translations::T("You're up to date!");
  renderer->drawText(title, 80, 25, theme.textColor);

  std::string versionText = Translations::T("Current:") + " " + currentInfo.currentVersion;
  if (currentInfo.updateAvailable) {
    versionText += " → " + Translations::T("Latest:") + " " + currentInfo.latestVersion;
  }
  renderer->drawText(versionText, 80, 50, theme.disabledText);

  renderer->drawLine(0, 80, width, 80, theme.separator);

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix) {
    betaToggleRect = Rect(width - 180, 90, 160, 30);

    Rect checkboxRect(betaToggleRect.x, betaToggleRect.y + 5, 20, 20);
    Color checkboxBorder = betaToggleHovered ? Color(100, 150, 255) : theme.disabledText;
    renderer->drawRect(checkboxRect, theme.windowBackground, true);
    renderer->drawRect(checkboxRect, checkboxBorder, false);

    if (betaEnabled) {
      renderer->drawRect(Rect(checkboxRect.x + 4, checkboxRect.y + 4, 12, 12),
        Color(100, 150, 255), true);
    }

    renderer->drawText(Translations::T("Include Beta Versions"),
      betaToggleRect.x + 25, betaToggleRect.y + 5, theme.textColor);
  }

  if (downloadState == DownloadState::Downloading || 
      downloadState == DownloadState::Connecting) {
    int progressY = 130;

    Color statusColor = theme.textColor;
    renderer->drawText(downloadStatus, 20, progressY, statusColor);

    progressBarRect = Rect(20, progressY + 30, width - 40, 30);
    renderer->drawProgressBar(progressBarRect, downloadProgress, theme);

    std::ostringstream pctStream;
    pctStream << (int)(downloadProgress * 100) << "%";
    renderer->drawText(pctStream.str(), width / 2 - 15, progressY + 38, theme.textColor);
  }
  else if (downloadState == DownloadState::Complete) {
    int messageY = 130;
    renderer->drawText(downloadStatus, 20, messageY, Color(100, 255, 100));
    renderer->drawText(Translations::T("Ready to install. Click Install to continue."), 
                      20, messageY + 30, theme.textColor);
  }
  else if (downloadState == DownloadState::Error) {
    renderer->drawText(downloadStatus, 20, 130, Color(255, 100, 100));
  }
  else if (currentInfo.updateAvailable && isMsix) {
    renderer->drawText(Translations::T("Microsoft Store Package Detected").c_str(), 20, 130, theme.textColor);
    renderer->drawText(Translations::T("Updates are managed through the Microsoft Store.").c_str(), 20, 160, theme.disabledText);
  }
  else if (currentInfo.updateAvailable && !currentInfo.releaseNotes.empty()) {
    renderer->drawText(Translations::T("What's New:").c_str(), 20, 130, theme.textColor);

    std::istringstream stream(currentInfo.releaseNotes);
    std::string line;
    int y = 160 - scrollOffset;
    int maxY = height - 100;

    while (std::getline(stream, line) && y < maxY) {
      if (y > 80) {
        renderer->drawText("• " + line, 30, y, theme.disabledText);
      }
      y += 25;
    }

    int contentHeight = y + scrollOffset - 160;
    if (contentHeight > (maxY - 160)) {
      scrollbarRect = Rect(width - 15, 130, 10, maxY - 130);
      renderer->drawRect(scrollbarRect, theme.scrollbarBg, true);

      int viewHeight = maxY - 160;
      float visibleRatio = (float)viewHeight / (float)contentHeight;
      visibleRatio = std::clamp(visibleRatio, 0.0f, 1.0f);
      int thumbHeight = (int)(scrollbarRect.height * visibleRatio);
      thumbHeight = std::clamp(thumbHeight, 30, scrollbarRect.height);

      int maxScroll = contentHeight - viewHeight;
      maxScroll = std::clamp(maxScroll, 0, INT_MAX);
      float scrollNorm = (maxScroll > 0) ? ((float)scrollOffset / (float)maxScroll) : 0.0f;
      scrollNorm = std::clamp(scrollNorm, 0.0f, 1.0f);
      int thumbY = scrollbarRect.y + (int)((scrollbarRect.height - thumbHeight) * scrollNorm);

      scrollThumbRect = Rect(scrollbarRect.x, thumbY, scrollbarRect.width, thumbHeight);
      Color thumbColor = scrollbarHovered ? Color(100, 100, 100) : Color(80, 80, 80);
      renderer->drawRect(scrollThumbRect, thumbColor, true);
    }
  }
  else if (!currentInfo.updateAvailable) {
    renderer->drawText(Translations::T("You have the latest version installed.").c_str(), 20, 150, theme.textColor);
    renderer->drawText(Translations::T("Check back later for updates.").c_str(), 20, 180, theme.disabledText);
  }

  int buttonY = height - 60;
  int buttonHeight = 40;

  if (downloadState == DownloadState::Idle && currentInfo.updateAvailable && !isMsix) {
    updateButtonRect = Rect(width - 280, buttonY, 120, buttonHeight);
    WidgetState updateState;
    updateState.rect = updateButtonRect;
    updateState.enabled = true;
    updateState.hovered = (hoveredButton == 1);
    updateState.pressed = (pressedButton == 1);
    std::string downloadText = betaEnabled ? 
      Translations::T("Download Beta") : Translations::T("Download");
    renderer->drawModernButton(updateState, theme, downloadText.c_str());

    skipButtonRect = Rect(width - 150, buttonY, 100, buttonHeight);
    WidgetState skipState;
    skipState.rect = skipButtonRect;
    skipState.enabled = true;
    skipState.hovered = (hoveredButton == 2);
    skipState.pressed = (pressedButton == 2);
    renderer->drawModernButton(skipState, theme, Translations::T("Skip").c_str());
  }
  else if (downloadState == DownloadState::Complete && !isMsix) {
    updateButtonRect = Rect(width - 280, buttonY, 120, buttonHeight);
    WidgetState installState;
    installState.rect = updateButtonRect;
    installState.enabled = true;
    installState.hovered = (hoveredButton == 1);
    installState.pressed = (pressedButton == 1);
    renderer->drawModernButton(installState, theme, Translations::T("Install").c_str());

    skipButtonRect = Rect(width - 150, buttonY, 100, buttonHeight);
    WidgetState cancelState;
    cancelState.rect = skipButtonRect;
    cancelState.enabled = true;
    cancelState.hovered = (hoveredButton == 2);
    cancelState.pressed = (pressedButton == 2);
    renderer->drawModernButton(cancelState, theme, Translations::T("Cancel").c_str());
  }
  else if ((downloadState == DownloadState::Idle && currentInfo.updateAvailable && isMsix) ||
           (downloadState == DownloadState::Idle && !currentInfo.updateAvailable) ||
           downloadState == DownloadState::Error) {
    closeButtonRect = Rect((width - 120) / 2, buttonY, 120, buttonHeight);
    WidgetState closeState;
    closeState.rect = closeButtonRect;
    closeState.enabled = true;
    closeState.hovered = (hoveredButton == 3);
    closeState.pressed = (pressedButton == 3);
    renderer->drawModernButton(closeState, theme, Translations::T("Close").c_str());
  }

  renderer->endFrame();
}

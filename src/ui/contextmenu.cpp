#ifdef _WIN32
#include <shlobj.h>
#include <windows.h>
extern HWND g_Hwnd;
#elif __APPLE__
#import <Cocoa/Cocoa.h>
extern NSWindow* g_nsWindow;
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
extern Display* g_display;
extern Window g_window;
#endif

#include "contextmenu.h"
#include "panelcontent.h"

extern HexData g_HexData;
extern ScrollbarState g_MainScrollbar;
extern SelectionState g_Selection;
extern RenderManager g_Renderer;
extern LeftPanelState g_LeftPanel;
extern MenuBar g_MenuBar;
extern int g_ScrollY;
extern int g_LinesPerPage;
extern int g_TotalLines;
extern AppOptions g_Options;
extern long long cursorBytePos;
extern int cursorNibblePos;
extern long long selectionLength;
extern size_t editingOffset;
extern int maxScrolls;

#ifdef _WIN32

HKEY ContextMenuRegistry::GetRootKey(UserRole role)
{
  switch (role)
  {
  case UserRole::CurrentUser:
    return HKEY_CURRENT_USER;
  default:
    return HKEY_CURRENT_USER;
  }
}

wchar_t *ContextMenuRegistry::GetRegistryPath(UserRole role)
{
  switch (role)
  {
  case UserRole::CurrentUser:
    return allocWideString(L"Software\\Classes\\*\\shell\\HexViewer");
  default:
    return allocWideString(L"Software\\Classes\\*\\shell\\HexViewer");
  }
}

bool ContextMenuRegistry::SetRegistryValue(HKEY hKey, const wchar_t *valueName, const wchar_t *data)
{
  LONG result = RegSetValueExW(
      hKey,
      valueName,
      0,
      REG_SZ,
      (const BYTE *)data,
      (DWORD)((wcsLen(data) + 1) * sizeof(wchar_t)));
  return result == ERROR_SUCCESS;
}

bool ContextMenuRegistry::DeleteRegistryKey(HKEY hRootKey, const wchar_t *subKey)
{
  LONG result = RegDeleteTreeW(hRootKey, subKey);
  return result == ERROR_SUCCESS || result == ERROR_FILE_NOT_FOUND;
}

bool ContextMenuRegistry::Register(const wchar_t *exePath, UserRole role)
{
  HKEY hKey = nullptr;
  HKEY hCommandKey = nullptr;
  bool success = false;

  HKEY rootKey = GetRootKey(role);
  wchar_t *registryPath = GetRegistryPath(role);

  LONG result = RegCreateKeyExW(
      rootKey,
      registryPath,
      0,
      nullptr,
      REG_OPTION_NON_VOLATILE,
      KEY_WRITE,
      nullptr,
      &hKey,
      nullptr);

  if (result != ERROR_SUCCESS)
  {
    platformFree(registryPath, (wcsLen(registryPath) + 1) * sizeof(wchar_t));
    return false;
  }

  if (!SetRegistryValue(hKey, nullptr, L"Open with Hex Viewer"))
  {
    RegCloseKey(hKey);
    platformFree(registryPath, (wcsLen(registryPath) + 1) * sizeof(wchar_t));
    return false;
  }

  size_t exePathLen = wcsLen(exePath);
  wchar_t *iconPath = (wchar_t *)platformAlloc((exePathLen + 3) * sizeof(wchar_t));
  wcsCopy(iconPath, exePath);
  iconPath[exePathLen] = L',';
  iconPath[exePathLen + 1] = L'0';
  iconPath[exePathLen + 2] = L'\0';

  SetRegistryValue(hKey, L"Icon", iconPath);
  platformFree(iconPath, (exePathLen + 3) * sizeof(wchar_t));

  result = RegCreateKeyExW(
      hKey,
      L"command",
      0,
      nullptr,
      REG_OPTION_NON_VOLATILE,
      KEY_WRITE,
      nullptr,
      &hCommandKey,
      nullptr);

  if (result == ERROR_SUCCESS)
  {
    size_t cmdLen = exePathLen + 8;
    wchar_t *command = (wchar_t *)platformAlloc(cmdLen * sizeof(wchar_t));
    wchar_t *p = command;

    *p++ = L'\"';
    wcsCopy(p, exePath);
    p += exePathLen;
    *p++ = L'\"';
    *p++ = L' ';
    *p++ = L'\"';
    *p++ = L'%';
    *p++ = L'1';
    *p++ = L'\"';
    *p = L'\0';

    success = SetRegistryValue(hCommandKey, nullptr, command);
    platformFree(command, cmdLen * sizeof(wchar_t));
    RegCloseKey(hCommandKey);
  }

  RegCloseKey(hKey);
  platformFree(registryPath, (wcsLen(registryPath) + 1) * sizeof(wchar_t));

  if (success)
  {
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
  }

  return success;
}

bool ContextMenuRegistry::Unregister(UserRole role)
{
  HKEY rootKey = GetRootKey(role);
  wchar_t *registryPath = GetRegistryPath(role);

  bool success = DeleteRegistryKey(rootKey, registryPath);
  platformFree(registryPath, (wcsLen(registryPath) + 1) * sizeof(wchar_t));

  if (success)
  {
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);
  }

  return success;
}

bool ContextMenuRegistry::IsRegistered(UserRole role)
{
  HKEY hKey = nullptr;
  HKEY rootKey = GetRootKey(role);
  wchar_t *registryPath = GetRegistryPath(role);

  LONG result = RegOpenKeyExW(
      rootKey,
      registryPath,
      0,
      KEY_READ,
      &hKey);

  platformFree(registryPath, (wcsLen(registryPath) + 1) * sizeof(wchar_t));

  if (result == ERROR_SUCCESS)
  {
    RegCloseKey(hKey);
    return true;
  }

  return false;
}
#endif

bool SetClipboardText(const char *text)
{
  if (!text)
    return false;

#ifdef _WIN32
  if (OpenClipboard(g_Hwnd))
  {
    EmptyClipboard();
    size_t textLen = strLen(text);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, textLen + 1);
    if (hMem)
    {
      char *pMem = (char *)GlobalLock(hMem);
      if (pMem)
      {
        memCopy(pMem, text, textLen + 1);
        GlobalUnlock(hMem);
        SetClipboardData(CF_TEXT, hMem);
      }
    }
    CloseClipboard();
    return true;
  }
  return false;
#elif __APPLE__
  @autoreleasepool
  {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    [pasteboard clearContents];
    NSString *nsText = [NSString stringWithUTF8String:text];
    [pasteboard setString:nsText forType:NSPasteboardTypeString];
    return true;
  }
#else
  if (!g_display)
    return false;

  Atom clipboard = XInternAtom(g_display, "CLIPBOARD", False);
  Atom utf8 = XInternAtom(g_display, "UTF8_STRING", False);

  XSetSelectionOwner(g_display, clipboard, g_window, CurrentTime);

  static char *clipboardData = nullptr;
  if (clipboardData)
  {
    platformFree(clipboardData, strLen(clipboardData) + 1);
  }
  clipboardData = allocString(text);

  return true;
#endif
}

char *GetClipboardText()
{
#ifdef _WIN32
  if (OpenClipboard(g_Hwnd))
  {
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData)
    {
      char *pData = (char *)GlobalLock(hData);
      if (pData)
      {
        char *result = allocString(pData);
        GlobalUnlock(hData);
        CloseClipboard();
        return result;
      }
    }
    CloseClipboard();
  }
  return nullptr;
#elif __APPLE__
  @autoreleasepool
  {
    NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
    NSString *nsText = [pasteboard stringForType:NSPasteboardTypeString];
    if (nsText)
    {
      return allocString([nsText UTF8String]);
    }
  }
  return nullptr;
#else
  return nullptr;
#endif
}

void InvalidateWindow()
{
#ifdef _WIN32
  InvalidateRect(g_Hwnd, NULL, FALSE);
#elif __APPLE__
#else
  if (g_display && g_window)
  {
    XClearWindow(g_display, g_window);
    XEvent exposeEvent;
    memSet(&exposeEvent, 0, sizeof(exposeEvent));
    exposeEvent.type = Expose;
    exposeEvent.xexpose.window = g_window;
    XSendEvent(g_display, g_window, False, ExposureMask, &exposeEvent);
    XFlush(g_display);
  }
#endif
}

void appendToBuffer(char *&buffer, size_t &capacity, size_t &length, const char *text)
{
  size_t textLen = strLen(text);
  size_t newLength = length + textLen;

  if (newLength >= capacity)
  {
    size_t newCapacity = capacity * 2;
    if (newCapacity < newLength + 1)
      newCapacity = newLength + 1;

    char *newBuffer = (char *)platformAlloc(newCapacity);
    if (buffer)
    {
      memCopy(newBuffer, buffer, length);
      platformFree(buffer, capacity);
    }
    buffer = newBuffer;
    capacity = newCapacity;
  }

  memCopy(buffer + length, text, textLen);
  length = newLength;
  buffer[length] = '\0';
}

void appendCharToBuffer(char *&buffer, size_t &capacity, size_t &length, char c)
{
  if (length + 1 >= capacity)
  {
    size_t newCapacity = capacity * 2;
    if (newCapacity < length + 2)
      newCapacity = length + 2;

    char *newBuffer = (char *)platformAlloc(newCapacity);
    if (buffer)
    {
      memCopy(newBuffer, buffer, length);
      platformFree(buffer, capacity);
    }
    buffer = newBuffer;
    capacity = newCapacity;
  }

  buffer[length++] = c;
  buffer[length] = '\0';
}

AppContextMenu::AppContextMenu()
{
  state.visible = false;
  state.hoveredIndex = -1;
  state.width = 200;
  state.openSubmenuIndex = -1;
}

void AppContextMenu::show(int x, int y)
{
  state.visible = true;
  state.x = x;
  state.y = y;
  state.hoveredIndex = -1;
  state.openSubmenuIndex = -1;
  state.items.clear();

  bool hasSelection = (selectionLength > 0);
  bool hasData = !g_HexData.isEmpty();
  bool hasCursor = (cursorBytePos != -1);

  {
    ContextMenuItem item;
    item.text = allocString("Copy");
    item.shortcut = allocString("Ctrl+C");
    item.enabled = hasSelection;
    item.checked = false;
    item.separator = false;
    item.id = ID_COPY;
    state.items.push_back(item);
  }

  {
    ContextMenuItem copyAsItem;
    copyAsItem.text = allocString("Copy As");
    copyAsItem.shortcut = nullptr;
    copyAsItem.enabled = hasSelection;
    copyAsItem.checked = false;
    copyAsItem.separator = false;
    copyAsItem.id = ID_COPY_AS;

    {
      ContextMenuItem sub;
      sub.text = allocString("Copy as Hex");
      sub.shortcut = nullptr;
      sub.enabled = hasSelection;
      sub.checked = false;
      sub.separator = false;
      sub.id = ID_COPY_HEX;
      copyAsItem.submenu.push_back(sub);
    }
    {
      ContextMenuItem sub;
      sub.text = allocString("Copy as Text");
      sub.shortcut = nullptr;
      sub.enabled = hasSelection;
      sub.checked = false;
      sub.separator = false;
      sub.id = ID_COPY_TEXT;
      copyAsItem.submenu.push_back(sub);
    }
    {
      ContextMenuItem sub;
      sub.text = allocString("Copy as C Array");
      sub.shortcut = nullptr;
      sub.enabled = hasSelection;
      sub.checked = false;
      sub.separator = false;
      sub.id = ID_COPY_CARRAY;
      copyAsItem.submenu.push_back(sub);
    }

    state.items.push_back(copyAsItem);
  }

  {
    ContextMenuItem item;
    item.text = allocString("Paste");
    item.shortcut = allocString("Ctrl+V");
    item.enabled = hasData && hasCursor;
    item.checked = false;
    item.separator = false;
    item.id = ID_PASTE;
    state.items.push_back(item);
  }

  {
    ContextMenuItem item;
    item.text = nullptr;
    item.shortcut = nullptr;
    item.enabled = true;
    item.checked = false;
    item.separator = true;
    item.id = 0;
    state.items.push_back(item);
  }

  {
    ContextMenuItem item;
    item.text = allocString("Select All");
    item.shortcut = allocString("Ctrl+A");
    item.enabled = hasData;
    item.checked = false;
    item.separator = false;
    item.id = ID_SELECT_ALL;
    state.items.push_back(item);
  }

  {
    ContextMenuItem item;
    item.text = nullptr;
    item.shortcut = nullptr;
    item.enabled = true;
    item.checked = false;
    item.separator = true;
    item.id = 0;
    state.items.push_back(item);
  }

  {
    ContextMenuItem item;
    item.text = allocString("Go to...");
    item.shortcut = allocString("Ctrl+G");
    item.enabled = hasData;
    item.checked = false;
    item.separator = false;
    item.id = ID_GOTO_OFFSET;
    state.items.push_back(item);
  }
  {
    ContextMenuItem item;
    item.text = allocString("Select Block...");
    item.shortcut = allocString("Ctrl+B");
    item.enabled = hasData;
    item.checked = false;
    item.separator = false;
    item.id = ID_SELECT_BLOCK;
    state.items.push_back(item);
  }

  {
    ContextMenuItem item;
    item.text = nullptr;
    item.shortcut = nullptr;
    item.enabled = true;
    item.checked = false;
    item.separator = true;
    item.id = 0;
    state.items.push_back(item);
  }

  {
    ContextMenuItem fillItem;
    fillItem.text = allocString("Fill Selection");
    fillItem.shortcut = nullptr;
    fillItem.enabled = hasSelection;
    fillItem.checked = false;
    fillItem.separator = false;
    fillItem.id = ID_FILL;

    {
      ContextMenuItem sub;
      sub.text = allocString("Fill with Zeros");
      sub.shortcut = nullptr;
      sub.enabled = hasSelection;
      sub.checked = false;
      sub.separator = false;
      sub.id = ID_FILL_ZEROS;
      fillItem.submenu.push_back(sub);
    }
    {
      ContextMenuItem sub;
      sub.text = allocString("Fill with 0xFF");
      sub.shortcut = nullptr;
      sub.enabled = hasSelection;
      sub.checked = false;
      sub.separator = false;
      sub.id = ID_FILL_FF;
      fillItem.submenu.push_back(sub);
    }
    {
      ContextMenuItem sub;
      sub.text = allocString("Fill with Pattern...");
      sub.shortcut = nullptr;
      sub.enabled = hasSelection;
      sub.checked = false;
      sub.separator = false;
      sub.id = ID_FILL_PATTERN;
      fillItem.submenu.push_back(sub);
    }

    state.items.push_back(fillItem);
  }

  {
    ContextMenuItem item;
    item.text = allocString("Add Bookmark");
    item.shortcut = nullptr;
    item.enabled = hasData && hasCursor;
    item.checked = false;
    item.separator = false;
    item.id = ID_ADD_BOOKMARK;
    state.items.push_back(item);
  }
}

void AppContextMenu::hide()
{
  state.visible = false;
  state.openSubmenuIndex = -1;
}

bool AppContextMenu::isVisible() const
{
  return state.visible;
}

const ContextMenuState &AppContextMenu::getState() const
{
  return state;
}

void AppContextMenu::handleMouseMove(int x, int y, RenderManager *renderer)
{
  if (!state.visible || !renderer)
    return;

  int oldHovered = state.hoveredIndex;
  state.hoveredIndex = renderer->getContextMenuHoveredItem(x, y, state);

  if (state.hoveredIndex >= 0 && state.hoveredIndex < (int)state.items.size())
  {
    const ContextMenuItem &item = state.items[state.hoveredIndex];
    if (!item.submenu.empty())
    {
      state.openSubmenuIndex = state.hoveredIndex;
    }
    else
    {
      state.openSubmenuIndex = -1;
    }
  }
}

int AppContextMenu::handleClick(int x, int y, RenderManager *renderer)
{
  if (!state.visible || !renderer)
    return -1;

  int clickedIndex = renderer->getContextMenuHoveredItem(x, y, state);

  if (clickedIndex >= 0 && clickedIndex < (int)state.items.size())
  {
    const ContextMenuItem &item = state.items[clickedIndex];

    if (item.enabled && !item.separator && item.submenu.empty())
    {
      hide();
      return item.id;
    }
  }

  if (!renderer->isPointInContextMenu(x, y, state))
  {
    hide();
  }

  return -1;
}

static void GoToOffsetCallback(int offset)
{
  if (offset >= 0 && offset < (int)g_HexData.getFileSize())
  {
    cursorBytePos = offset;
    editingOffset = offset;
    cursorNibblePos = 0;

    int bytesPerLine = g_HexData.getCurrentBytesPerLine();
    long long targetLine = offset / bytesPerLine;

    int maxScroll = g_TotalLines - g_LinesPerPage;
    if (maxScroll < 0)
      maxScroll = 0;

    g_ScrollY = (int)(targetLine - g_LinesPerPage / 2);
    if (g_ScrollY < 0)
      g_ScrollY = 0;
    if (g_ScrollY > maxScroll)
      g_ScrollY = maxScroll;

    if (maxScroll > 0)
      g_MainScrollbar.position = (float)g_ScrollY / (float)maxScroll;
    else
      g_MainScrollbar.position = 0.0f;

    int leftPanelWidth = g_LeftPanel.visible ? g_LeftPanel.width : 0;
    g_Renderer.UpdateHexMetrics(leftPanelWidth, g_MenuBar.getHeight());

    InvalidateWindow();
  }
}

long long ParseNumber(const char *text, int numberFormat)
{
  if (!text)
    return 0;

  while (*text == ' ' || *text == '\t' || *text == '\n' || *text == '\r')
    ++text;

  int base = 10;

  switch (numberFormat)
  {
  case 0:
    base = 16;
    break;
  case 1:
    base = 10;
    break;
  case 2:
    base = 8;
    break;
  default:
    base = 10;
    break;
  }

  long long result = 0;
  bool negative = false;

  if (*text == '+')
  {
    ++text;
  }
  else if (*text == '-')
  {
    negative = true;
    ++text;
  }

  if (base == 16 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    text += 2;

  while (*text)
  {
    char c = *text++;
    int v = -1;

    if (c >= '0' && c <= '9')
      v = c - '0';
    else if (c >= 'A' && c <= 'Z')
      v = 10 + (c - 'A');
    else if (c >= 'a' && c <= 'z')
      v = 10 + (c - 'a');

    if (v < 0 || v >= base)
      break;

    result = result * base + v;
  }

  return negative ? -result : result;
}

void AppContextMenu::executeAction(int actionId)
{
  switch (actionId)
  {
  case ID_COPY:
  case ID_COPY_HEX:
  {
    if (selectionLength > 0)
    {
      size_t capacity = 1024;
      size_t length = 0;
      char *hexString = (char *)platformAlloc(capacity);
      hexString[0] = '\0';

      long long start = cursorBytePos;
      long long end = start + selectionLength;

      for (long long i = start; i < end && i < (long long)g_HexData.getFileSize(); i++)
      {
        uint8_t byte = g_HexData.readByte(i);
        char hex[4];
        byteToHex(byte, hex);
        hex[2] = ' ';
        hex[3] = '\0';
        appendToBuffer(hexString, capacity, length, hex);
      }

      if (length > 0 && hexString[length - 1] == ' ')
      {
        hexString[length - 1] = '\0';
      }

      SetClipboardText(hexString);
      platformFree(hexString, capacity);
    }
    break;
  }

  case ID_COPY_TEXT:
  {
    if (selectionLength > 0)
    {
      size_t capacity = 1024;
      size_t length = 0;
      char *textString = (char *)platformAlloc(capacity);
      textString[0] = '\0';

      long long start = cursorBytePos;
      long long end = start + selectionLength;

      for (long long i = start; i < end && i < (long long)g_HexData.getFileSize(); i++)
      {
        uint8_t byte = g_HexData.readByte(i);
        char c = (byte >= 32 && byte <= 126) ? (char)byte : '.';
        appendCharToBuffer(textString, capacity, length, c);
      }

      SetClipboardText(textString);
      platformFree(textString, capacity);
    }
    break;
  }

  case ID_COPY_CARRAY:
  {
    if (selectionLength > 0)
    {
      size_t capacity = 4096;
      size_t length = 0;
      char *cArrayString = (char *)platformAlloc(capacity);
      cArrayString[0] = '\0';

      appendToBuffer(cArrayString, capacity, length, "unsigned char data[] = {\n    ");

      long long start = cursorBytePos;
      long long end = start + selectionLength;
      int bytesPerLine = 12;
      int count = 0;

      for (long long i = start; i < end && i < (long long)g_HexData.getFileSize(); i++)
      {
        uint8_t byte = g_HexData.readByte(i);
        char hex[8];
        hex[0] = '0';
        hex[1] = 'x';
        byteToHex(byte, hex + 2);
        hex[4] = '\0';
        appendToBuffer(cArrayString, capacity, length, hex);

        if (i < end - 1 && i < (long long)g_HexData.getFileSize() - 1)
        {
          appendToBuffer(cArrayString, capacity, length, ", ");
          count++;

          if (count >= bytesPerLine)
          {
            appendToBuffer(cArrayString, capacity, length, "\n    ");
            count = 0;
          }
        }
      }

      appendToBuffer(cArrayString, capacity, length, "\n};");
      SetClipboardText(cArrayString);
      platformFree(cArrayString, capacity);
    }
    break;
  }

  case ID_PASTE:
  {
    if (cursorBytePos != -1)
    {
      char *clipText = GetClipboardText();
      if (clipText)
      {
        Vector<uint8_t> bytes;
        size_t pos = 0;
        size_t clipLen = strLen(clipText);

        while (pos < clipLen)
        {
          while (pos < clipLen &&
                 (clipText[pos] == ' ' || clipText[pos] == ',' ||
                  clipText[pos] == '\n' || clipText[pos] == '\r' ||
                  clipText[pos] == '\t'))
          {
            pos++;
          }

          if (pos >= clipLen)
            break;

          if (pos + 1 < clipLen && clipText[pos] == '0' &&
              (clipText[pos + 1] == 'x' || clipText[pos + 1] == 'X'))
          {
            pos += 2;
          }

          if (pos + 1 < clipLen &&
              isXDigit(clipText[pos]) && isXDigit(clipText[pos + 1]))
          {
            uint8_t byte = (hexDigitToInt(clipText[pos]) << 4) |
                           hexDigitToInt(clipText[pos + 1]);
            bytes.push_back(byte);
            pos += 2;
          }
          else
          {
            pos++;
          }
        }

        long long pastePos = cursorBytePos;
        const size_t fileSize = g_HexData.getFileSize();

        for (size_t i = 0; i < bytes.size() && pastePos < static_cast<long long>(fileSize); ++i)
        {
          g_HexData.editByte(static_cast<size_t>(pastePos), bytes[i]);
          ++pastePos;
        }

        long long maxPos = (fileSize > 0) ? static_cast<long long>(fileSize - 1) : 0LL;
        cursorBytePos = clampLL(pastePos, 0LL, maxPos);
        editingOffset = cursorBytePos;

        platformFree(clipText, strLen(clipText) + 1);
        InvalidateWindow();
      }
    }
    break;
  }

  case ID_SELECT_ALL:
    if (!g_HexData.isEmpty())
    {
      cursorBytePos = 0;
      selectionLength = g_HexData.getFileSize();
      InvalidateWindow();
    }
    break;

  case ID_GOTO_OFFSET:
  {
#ifdef _WIN32
    SearchDialogs::ShowGoToDialog(g_Hwnd, g_Options.darkMode, GoToOffsetCallback, nullptr);
#elif __APPLE__
    SearchDialogs::ShowGoToDialog(
        (NativeWindow)g_nsWindow,
        g_Options.darkMode,
        [](int offset)
        {
          if (offset >= 0 && offset < (int)g_HexData.getFileSize())
          {
            cursorBytePos = offset;
            editingOffset = offset;
            cursorNibblePos = 0;

            int bytesPerLine = g_HexData.getCurrentBytesPerLine();
            int targetRow = offset / bytesPerLine;
            g_ScrollY = clamp(targetRow - 5, 0, maxScrolls);

            InvalidateWindow();
          }
        });
#else
    SearchDialogs::ShowGoToDialog(
        (void *)g_window,
        g_Options.darkMode,
        [](int offset)
        {
          if (offset >= 0 && offset < (int)g_HexData.getFileSize())
          {
            cursorBytePos = offset;
            editingOffset = offset;
            cursorNibblePos = 0;

            int bytesPerLine = g_HexData.getCurrentBytesPerLine();
            int targetRow = offset / bytesPerLine;
            g_ScrollY = clamp(targetRow - 5, 0, maxScrolls);

            InvalidateWindow();
          }
        });
#endif
    break;
  }

  case ID_FILL_ZEROS:
  {
    if (selectionLength > 0)
    {
      long long start = cursorBytePos;
      long long end = start + selectionLength;

      for (long long i = start; i < end && i < (long long)g_HexData.getFileSize(); i++)
      {
        g_HexData.editByte(i, 0x00);
      }

      InvalidateWindow();
    }
    break;
  }

  case ID_FILL_FF:
  {
    if (selectionLength > 0)
    {
      long long start = cursorBytePos;
      long long end = start + selectionLength;

      for (long long i = start; i < end && i < (long long)g_HexData.getFileSize(); i++)
      {
        g_HexData.editByte(i, 0xFF);
      }

      InvalidateWindow();
    }
    break;
  }

  case ID_FILL_PATTERN:
  {
    if (selectionLength > 0)
    {
      uint8_t patternBytes[] = {0xAA, 0xBB, 0xCC, 0xDD};
      int patternLen = 4;
      long long start = cursorBytePos;
      long long end = start + selectionLength;
      size_t patternIndex = 0;

      for (long long i = start; i < end && i < (long long)g_HexData.getFileSize(); i++)
      {
        g_HexData.editByte(i, patternBytes[patternIndex]);
        patternIndex = (patternIndex + 1) % patternLen;
      }

      InvalidateWindow();
    }
    break;
  }

  case ID_ADD_BOOKMARK:
  {
    if (cursorBytePos != -1)
    {
      char defaultName[64];
      strCopy(defaultName, "Offset 0x");
      char offsetStr[32];
      itoaHex(cursorBytePos, offsetStr, 32);
      strCat(defaultName, offsetStr);

#ifdef _WIN32
      SearchDialogs::ShowInputDialog(
        g_Hwnd,
        "Add Bookmark",
        "Enter bookmark name:",
        defaultName,
        g_Options.darkMode,
        [](const char* bookmarkName)
        {
          if (bookmarkName && bookmarkName[0] != '\0')
          {
            Bookmark newBookmark;
            newBookmark.byteOffset = cursorBytePos;
            strCopy(newBookmark.name, bookmarkName);

            Color colors[] = {
                Color(255, 100, 100),
                Color(100, 255, 100),
                Color(100, 100, 255),
                Color(255, 255, 100),
                Color(255, 100, 255),
                Color(100, 255, 255) };
            int colorIndex = g_Bookmarks.bookmarks.size() % 6;
            newBookmark.color = colors[colorIndex];

            g_Bookmarks.bookmarks.push_back(newBookmark);
            g_Bookmarks.selectedIndex = g_Bookmarks.bookmarks.size() - 1;

            InvalidateWindow();
          }
        },
        nullptr);

#elif defined(__APPLE__)
      SearchDialogs::ShowInputDialog(
        (NativeWindow)g_nsWindow,
        "Add Bookmark",
        "Enter bookmark name:",
        defaultName,
        g_Options.darkMode,
        [](const std::string& bookmarkName)
        {
          if (!bookmarkName.empty())
          {
            Bookmark newBookmark;
            newBookmark.byteOffset = cursorBytePos;
            strCopy(newBookmark.name, bookmarkName.c_str());

            Color colors[] = {
                Color(255, 100, 100),
                Color(100, 255, 100),
                Color(100, 100, 255),
                Color(255, 255, 100),
                Color(255, 100, 255),
                Color(100, 255, 255) };
            int colorIndex = g_Bookmarks.bookmarks.size() % 6;
            newBookmark.color = colors[colorIndex];

            g_Bookmarks.bookmarks.push_back(newBookmark);
            g_Bookmarks.selectedIndex = g_Bookmarks.bookmarks.size() - 1;

            InvalidateWindow();
          }
        });

#else
      SearchDialogs::ShowInputDialog(
        (void*)g_window,
        "Add Bookmark",
        "Enter bookmark name:",
        defaultName,
        g_Options.darkMode,
        [](const std::string& bookmarkName)
        {
          if (!bookmarkName.empty())
          {
            Bookmark newBookmark;
            newBookmark.byteOffset = cursorBytePos;
            strCopy(newBookmark.name, bookmarkName.c_str());

            Color colors[] = {
                Color(255, 100, 100),
                Color(100, 255, 100),
                Color(100, 100, 255),
                Color(255, 255, 100),
                Color(255, 100, 255),
                Color(100, 255, 255) };
            int colorIndex = g_Bookmarks.bookmarks.size() % 6;
            newBookmark.color = colors[colorIndex];

            g_Bookmarks.bookmarks.push_back(newBookmark);
            g_Bookmarks.selectedIndex = g_Bookmarks.bookmarks.size() - 1;

            InvalidateWindow();
          }
        });
#endif
    }
    break;
  }
  case ID_SELECT_BLOCK:
  {
    long long initialStart = -1;
    long long initialLength = -1;

    char debugMsg[256];
    char temp[64];

    if (selectionLength > 0)
    {
      long long selStart = cursorBytePos - (selectionLength - 1);
      if (selStart < 0) selStart = 0;

      initialStart = selStart;
      initialLength = selectionLength;

    }

    else if (cursorBytePos >= 0)
    {
      initialStart = cursorBytePos;
      initialLength = 0;
    
  }
#ifdef _WIN32
    ShowSelectBlockDialog(
      g_Hwnd,
      g_Options.darkMode,
      [](const char* startStr,
        const char* endOrLengthStr,
        bool useLength,
        int numberFormat)
      {
        long long start = ParseNumber(startStr, numberFormat);
        long long value = ParseNumber(endOrLengthStr, numberFormat);

        long long length = useLength ? value : (value - start);

        if (start >= 0 &&
          length > 0 &&
          start < (long long)g_HexData.getFileSize())
        {
          long long end = start + length;
          long long maxEnd = (long long)g_HexData.getFileSize();
          if (end > maxEnd)
            end = maxEnd;

          cursorBytePos = start;
          selectionLength = end - start;
          editingOffset = start;
          cursorNibblePos = 0;

          int bytesPerLine = g_HexData.getCurrentBytesPerLine();
          int targetRow = start / bytesPerLine;
          g_ScrollY = clamp(targetRow - 5, 0, maxScrolls);

          InvalidateWindow();
        }
      },
      nullptr,
      initialStart,
      initialLength);

#elif __APPLE__
    ShowSelectBlockDialog(
      (NativeWindow)g_nsWindow,
      g_Options.darkMode,
      [](const std::string& startStr,
        const std::string& endOrLengthStr,
        bool useLength,
        int numberFormat)
      {
        long long start = ParseNumber(startStr.c_str(), numberFormat);
        long long value = ParseNumber(endOrLengthStr.c_str(), numberFormat);

        long long length = useLength ? value : (value - start);

        if (start >= 0 &&
          length > 0 &&
          start < (long long)g_HexData.getFileSize())
        {
          long long end = start + length;
          long long maxEnd = (long long)g_HexData.getFileSize();
          if (end > maxEnd)
            end = maxEnd;

          cursorBytePos = start;
          selectionLength = end - start;
          editingOffset = start;
          cursorNibblePos = 0;

          int bytesPerLine = g_HexData.getCurrentBytesPerLine();
          int targetRow = start / bytesPerLine;
          g_ScrollY = clamp(targetRow - 5, 0, maxScrolls);

          InvalidateWindow();
        }
      },
      initialStart,
      initialLength);

#else
    ShowSelectBlockDialog(
      (void*)g_window,
      g_Options.darkMode,
      [](const std::string& startStr,
        const std::string& endOrLengthStr,
        bool useLength,
        int numberFormat)
      {
        long long start = ParseNumber(startStr.c_str(), numberFormat);
        long long value = ParseNumber(endOrLengthStr.c_str(), numberFormat);

        long long length = useLength ? value : (value - start);

        if (start >= 0 &&
          length > 0 &&
          start < (long long)g_HexData.getFileSize())
        {
          long long end = start + length;
          long long maxEnd = (long long)g_HexData.getFileSize();
          if (end > maxEnd)
            end = maxEnd;

          cursorBytePos = start;
          selectionLength = end - start;
          editingOffset = start;
          cursorNibblePos = 0;

          int bytesPerLine = g_HexData.getCurrentBytesPerLine();
          int targetRow = start / bytesPerLine;
          g_ScrollY = clamp(targetRow - 5, 0, maxScrolls);

          InvalidateWindow();
        }
      },
      initialStart,
      initialLength);
#endif

    break;
  }
  }
}

#include "pch.h"
#include "ShellEx.h"
#include <VersionHelpers.h>
#include <strsafe.h>
#include <comutil.h>
#include <memory>
#include "resource.h"

#pragma comment( lib, "comsuppwd.lib")

extern HINSTANCE g_hInst;
extern long g_cDllRef;

#define IDM_DISPLAY             0

HexViewer::HexViewer(void) :m_cRef(1),

	m_pszMenuText(L"Open with HexViewer"),
	m_pszVerb("HexViewer"),
	m_pwszVerb(L"HexViewer"),
	m_pszVerbCanonicalName("HexViewer"),
	m_pwszVerbCanonicalName(L"HexViewer"),
	m_pszVerbHelpText("Open file in HexViewer"),
	m_pwszVerbHelpText(L"HexViewer"),
	m_nFiles(0)

    
{
    InterlockedIncrement(&g_cDllRef);
    m_hMenuBmp = LoadImage(g_hInst, MAKEINTRESOURCE(IDB_OK),
        IMAGE_BITMAP, 0, 0, LR_DEFAULTSIZE | LR_LOADTRANSPARENT);
    if (m_hMenuBmp == NULL) {
        DWORD dwError = GetLastError();
        LPVOID lpMsgBuf;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            dwError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&lpMsgBuf,
            0, NULL);
        MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK | MB_ICONERROR);
        LocalFree(lpMsgBuf);
    }
}

HexViewer::~HexViewer(void)
{
    LogMessage(L"HexViewer destructor called");
    if (m_hMenuBmp)
    {
        DeleteObject(m_hMenuBmp);
        m_hMenuBmp = NULL;
    }
    InterlockedDecrement(&g_cDllRef);
    
   
}


IFACEMETHODIMP_(ULONG) HexViewer::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}
IFACEMETHODIMP_(ULONG) HexViewer::Release()
{
    ULONG cRef = InterlockedDecrement(&m_cRef);
    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}
IFACEMETHODIMP HexViewer::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT pDataObj, HKEY hKeyProgID)
{
    if (NULL == pDataObj)
    {
        return E_INVALIDARG;
    }

    HRESULT hr = E_FAIL;

    FORMATETC fe = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stm;

    if (SUCCEEDED(pDataObj->GetData(&fe, &stm)))
    {
        HDROP hDrop = static_cast<HDROP>(GlobalLock(stm.hGlobal));
        if (hDrop != NULL)
        {
            UINT nFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            if (nFiles == 1)
            {
                if (0 != DragQueryFile(hDrop, 0, m_szSelectedFile,
                    ARRAYSIZE(m_szSelectedFile)))
                {
                    hr = S_OK;
                }
            }

            GlobalUnlock(stm.hGlobal);
        }

        ReleaseStgMedium(&stm);
    }

    return hr;
}
IFACEMETHODIMP HexViewer::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] =
    {
        QITABENT(HexViewer, IContextMenu),
        QITABENT(HexViewer, IShellExtInit),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP HexViewer::QueryContextMenu(
  HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
  LogMessage(L"QueryContextMenu entered");

  if (uFlags & CMF_DEFAULTONLY) {
    LogMessage(L"CMF_DEFAULTONLY set; not adding items");
    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(0));
  }

  MENUITEMINFO mii = { sizeof(mii) };
  mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_ID | MIIM_STATE /*| MIIM_BITMAP*/;
  mii.wID = idCmdFirst + IDM_DISPLAY;
  mii.fType = MFT_STRING;
  mii.fState = MFS_ENABLED;

  if (!m_pszMenuText || *m_pszMenuText == L'\0') {
    LogMessage(L"m_pszMenuText is empty; using fallback text");
    m_pszMenuText = L"Open with HexViewer";
  }

  size_t len = wcslen(m_pszMenuText) + 1;
  std::unique_ptr<WCHAR[]> pszMenuTextCopy(new WCHAR[len]);
  wcscpy_s(pszMenuTextCopy.get(), len, m_pszMenuText);
  mii.dwTypeData = pszMenuTextCopy.get();


  if (!InsertMenuItem(hMenu, indexMenu, TRUE, &mii)) {
    DWORD err = GetLastError();
    LogMessage(L"InsertMenuItem (text) failed, error=" + std::to_wstring(err));
    return HRESULT_FROM_WIN32(err);
  }
  LogMessage(L"Inserted menu item: Open with HexViewer");

  /*
  MENUITEMINFO sep = { sizeof(sep) };
  sep.fMask = MIIM_TYPE;
  sep.fType = MFT_SEPARATOR;
  if (!InsertMenuItem(hMenu, indexMenu + 1, TRUE, &sep)) {
      DWORD err = GetLastError();
      LogMessage(L"InsertMenuItem (separator) failed, error=" + std::to_wstring(err));
      return HRESULT_FROM_WIN32(err);
  }
  */

  return MAKE_HRESULT(SEVERITY_SUCCESS, 0, USHORT(1));
}


IFACEMETHODIMP HexViewer::InvokeCommand(LPCMINVOKECOMMANDINFO pici)
{
  LogMessage(L"InvokeCommand entered");

  if (!HIWORD(pici->lpVerb)) {
    UINT idCmd = LOWORD(pici->lpVerb);
    if (idCmd != IDM_DISPLAY) {
      LogMessage(L"InvokeCommand: wrong idCmd");
      return E_FAIL;
    }

    if (m_szSelectedFile[0] == L'\0') {
      LogMessage(L"InvokeCommand: no selected file");
      return E_FAIL;
    }

    LogMessage(std::wstring(L"Selected file: ") + m_szSelectedFile);

    WCHAR szModuleDir[MAX_PATH] = {};
    if (!GetModuleFileNameW(g_hInst, szModuleDir, ARRAYSIZE(szModuleDir))) {
      DWORD err = GetLastError();
      LogMessage(L"GetModuleFileName failed, error=" + std::to_wstring(err));
      return HRESULT_FROM_WIN32(err);
    }
    PathRemoveFileSpecW(szModuleDir);

    WCHAR szHexViewer[MAX_PATH] = {};
    if (!PathCombineW(szHexViewer, szModuleDir, L"HexViewer.exe")) {
      LogMessage(L"PathCombine failed");
      return E_FAIL;
    }

    LogMessage(std::wstring(L"Exe path: ") + szHexViewer);

    HINSTANCE hRes = ShellExecuteW(
      pici->hwnd,
      L"open",
      szHexViewer,
      m_szSelectedFile,
      NULL,
      SW_SHOWNORMAL
    );

    if ((INT_PTR)hRes <= 32) {
      DWORD err = GetLastError();
      LogMessage(L"ShellExecute failed, error=" + std::to_wstring(err));
      return HRESULT_FROM_WIN32(err);
    }

    LogMessage(L"ShellExecute succeeded");
    return S_OK;
  }

  LogMessage(L"InvokeCommand: verb not handled");
  return E_FAIL;
}



IFACEMETHODIMP HexViewer::GetCommandString(UINT_PTR idCommand,
    UINT uFlags, UINT* pwReserved, LPSTR pszName, UINT cchMax)
{	
    HRESULT hr = E_INVALIDARG;

    if (idCommand == IDM_DISPLAY)
    {
        switch (uFlags)
        {
        case GCS_HELPTEXTW:
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax,
                m_pwszVerbHelpText);
            break;

        case GCS_VERBW:
            hr = StringCchCopy(reinterpret_cast<PWSTR>(pszName), cchMax,
                m_pwszVerbCanonicalName);
            break;

        default:
            hr = S_OK;
        }
    }


    return hr;
}
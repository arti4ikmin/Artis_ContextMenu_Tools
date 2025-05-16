#include "pch.h"
#include "Registerer.h"

#include <shlobj_core.h>

#include "Guids.h"
#include <Shlwapi.h> // SHDeleteKey
#include <strsafe.h> // StringCchPrintfW

extern HINSTANCE g_hInstDll; // defined in dllmain

#define SCPW StringCchPrintfW

auto HANDLER_NAME = L"CppToolsHandler";
auto HANDLER_DESC = L"C++ Tools Context Menu Handler";

HRESULT SetRegistryValue(const HKEY hKeyRoot, const WCHAR* pszSubKey, const WCHAR* pszValueName, const WCHAR* pszData) {
    HKEY hKey;
    LONG lResult = RegCreateKeyExW(hKeyRoot, pszSubKey, 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &hKey, nullptr);
    if (lResult != ERROR_SUCCESS) {
        return HRESULT_FROM_WIN32(lResult);
    }
    lResult = RegSetValueExW(hKey, pszValueName, 0, REG_SZ, reinterpret_cast<const BYTE*>(pszData), (wcslen(pszData) + 1) * sizeof(WCHAR));
    RegCloseKey(hKey);
    return HRESULT_FROM_WIN32(lResult);
}

// registrationLogic
HRESULT RegisterContextMenuHandler() {
    WCHAR szClsid[MAX_PATH];
    StringFromGUID2(CLSID_ContextMenuHandler, szClsid, ARRAYSIZE(szClsid));

    WCHAR szModulePath[MAX_PATH];
    GetModuleFileNameW(g_hInstDll, szModulePath, ARRAYSIZE(szModulePath));

    WCHAR szKeyPath[MAX_PATH];


    
    // 1 reg the COM (CLSID)
    // TODO: add a if succeed foreach StringCchPrintfW (SCPW) to prevent "Function result of type 'HRESULT' should be used"
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s", szClsid);
    HRESULT hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, HANDLER_DESC);
    if (FAILED(hr)) return hr;

    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s\\InprocServer32", szClsid);
    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szModulePath);
     if (FAILED(hr)) return hr;
    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, L"ThreadingModel", L"Apartment");
     if (FAILED(hr)) return hr;


    // 2 reg as a contextMenuHandler for all
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"*\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);
    if (FAILED(hr)) return hr;

    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"AllFilesystemObjects\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);

    // folders TODO: REMOVE IF NO USE IN FUTURE
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);
    // falure is here not critical ig

    // folder bg
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\Background\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);
    // same here

    //TODO: COMMENT HRs IF SMTH BREAKS
    
    // HKCR\.txt\shellex\... if required in future, add here

    // send event
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    return S_OK;
}

// unregistrationLogic
HRESULT UnregisterContextMenuHandler() {
    WCHAR szClsid[MAX_PATH];
    StringFromGUID2(CLSID_ContextMenuHandler, szClsid, ARRAYSIZE(szClsid));

    WCHAR szKeyPath[MAX_PATH];

    // 1 del the handler keys
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"*\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath); // ig errors should be ignored, as they dont exist

    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"AllFilesystemObjects\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath);
    
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath);

    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\Background\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath);


    
    // 2 delete the CLSID key
    StringCchPrintfW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s", szClsid);
    const LONG lResult = SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath); // recursive

    // sand event
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    return lResult == ERROR_SUCCESS || lResult == ERROR_FILE_NOT_FOUND ? S_OK : HRESULT_FROM_WIN32(lResult);
}
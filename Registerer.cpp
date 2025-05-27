#include "pch.h"
#include "Registerer.h"

#include <shlobj_core.h>

#include "Guids.h"
#include <Shlwapi.h> // SHDeleteKey
#include <strsafe.h> // StringCchPrintfW

extern HINSTANCE g_hInstDll; // defined in dllmain

#define SCPW StringCchPrintfW
#define MB MessageBoxW

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
    HRESULT hr = S_OK;
    
    // 1 reg the COM (CLSID)
    
    HRESULT hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s", szClsid);
    if (FAILED(hrTemp)) {
        //MB(nullptr, L"Failed to SCPW CLSID\\%s", L"Fatal Error", MB_OK | MB_ICONWARNING);
        return hrTemp;
    }

    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, HANDLER_DESC);
    if (FAILED(hr)) return hr;


    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s\\InprocServer32", szClsid);
    if (FAILED(hrTemp)) {
        //MB(nullptr, L"Failed to SCPW CLSID\\%s\\InprocServer32", L"Fatal Error", MB_OK | MB_ICONWARNING);
        return hrTemp;
    }

    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szModulePath);
     if (FAILED(hr)) return hr;
    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, L"ThreadingModel", L"Apartment");
     if (FAILED(hr)) return hr;


    // 2 reg as a contextMenuHandler for all


    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"*\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        //MB(nullptr, L"Failed to SCPW \\shellex\\ContextMenuHandlers\\%s", L"Fatal Error", MB_OK | MB_ICONWARNING);
        return hrTemp;
    }

    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);
    if (FAILED(hr)) return hr;


    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"AllFilesystemObjects\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        //MB(nullptr, L"Failed to SCPW AllFilesystemObjects\\shellex\\ContextMenuHandlers\\%s", L"Fatal Error", MB_OK | MB_ICONWARNING);
        return hrTemp;
    }

    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);

    // folders TODO: REMOVE IF NO USE IN FUTURE

    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        //MB(nullptr, L"Failed to SCPW Directory\\shellex\\ContextMenuHandlers\\%s", L"Fatal Error", MB_OK | MB_ICONWARNING);
        return hrTemp;
    }

    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);
    // falure is here not critical ig (SetRegistryValue)

    // folder bg

    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\Background\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        //MB(nullptr, L"Failed to SCPW Directory\\Background\\shellex\\ContextMenuHandlers\\%s", L"Fatal Error", MB_OK | MB_ICONWARNING);
        return hrTemp;
    }

    hr = SetRegistryValue(HKEY_CLASSES_ROOT, szKeyPath, nullptr, szClsid);
    // same here (SetRegistryValue)
    
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
    HRESULT hrTemp = S_OK;

    // 1 del the handler keys

    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"*\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        return hrTemp;
    }

    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath); // ig errors should be ignored, as they dont exist


    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"AllFilesystemObjects\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        return hrTemp;
    }

    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath);
    

    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        return hrTemp;
    }

    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath);


    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"Directory\\Background\\shellex\\ContextMenuHandlers\\%s", HANDLER_NAME);
    if (FAILED(hrTemp)) {
        return hrTemp;
    }

    SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath);


    
    // 2 delete the CLSID key

    hrTemp = SCPW(szKeyPath, ARRAYSIZE(szKeyPath), L"CLSID\\%s", szClsid);
    if (FAILED(hrTemp)) {
        return hrTemp;
    }

    const LONG lResult = SHDeleteKeyW(HKEY_CLASSES_ROOT, szKeyPath); // recursive

    // send event
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, nullptr, nullptr);

    return lResult == ERROR_SUCCESS || lResult == ERROR_FILE_NOT_FOUND ? S_OK : HRESULT_FROM_WIN32(lResult);
}
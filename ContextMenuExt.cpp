#include "pch.h"
#include "ContextMenuExt.h"
#include "Guids.h"
#include <vector>
#include <string>
#include <Shlwapi.h>
#include <strsafe.h>

#include "Tools/ExtChanger.h"
#include "Tools/UploadToTmpFiles.h"
#include "Tools/PathCopier.h"
#include "Tools/FileHasher.h"
//#include "Tools/NetworkUtils.h"

// Code is now partially inspired by https://github.com/microsoft/PowerToys/blob/main/src/modules/FileLocksmith/FileLocksmithExt/ExplorerCommand.cpp (cuz Im too retareded to make a proper context menu)

extern HINSTANCE g_hInstDll;
extern long g_cRefModule;

CContextMenuExt::CContextMenuExt() : m_cRef(1) {
    InterlockedIncrement(&g_cRefModule);
}

CContextMenuExt::~CContextMenuExt() {
    InterlockedDecrement(&g_cRefModule);
}

// IUnknown
IFACEMETHODIMP CContextMenuExt::QueryInterface(REFIID riid, void** ppv) {
    static const QITAB qit[] = {
        QITABENT(CContextMenuExt, IContextMenu),
        QITABENT(CContextMenuExt, IShellExtInit),
        { nullptr },
    };
    return QISearch(this, qit, riid, ppv);
}

IFACEMETHODIMP_(ULONG) CContextMenuExt::AddRef() {
    return InterlockedIncrement(&m_cRef);
}

IFACEMETHODIMP_(ULONG) CContextMenuExt::Release() {
    const long cRef = InterlockedDecrement(&m_cRef);
    if (cRef == 0) {
        delete this;
    }
    return cRef;
}

// IShellExtInit
IFACEMETHODIMP CContextMenuExt::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    if (pdtobj == nullptr) {
        return E_INVALIDARG;
    }

    m_selectedFiles.clear();

    FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg = { TYMED_HGLOBAL };

    // CF_HDROP data in the data obj
    if (FAILED(pdtobj->GetData(&fmt, &stg))) {
        // when CF_HDROP is not available the user prob rightclicked the background
        // or something else we dunno... msgbox used to be here
        return E_INVALIDARG;
    }

    // getter
    const auto hdrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
    if (nullptr == hdrop) {
        ReleaseStgMedium(&stg);
        return E_OUTOFMEMORY;
    }

    // files count
    const UINT uNumFiles = DragQueryFileW(hdrop, 0xFFFFFFFF, nullptr, 0);
    if (uNumFiles == 0) {
         GlobalUnlock(stg.hGlobal);
         ReleaseStgMedium(&stg);
         return E_INVALIDARG; // shouldnt happen if GetData succeeded
    }

    m_selectedFiles.reserve(uNumFiles);

    // paths foreach
    for (UINT i = 0; i < uNumFiles; ++i) {
        if (const UINT cch = DragQueryFileW(hdrop, i, nullptr, 0); cch > 0) { //buffsize (cch)
            std::wstring filePath(cch + 1, L'\0'); // +1 for null terminator
            if (DragQueryFileW(hdrop, i, &filePath[0], filePath.size())) {
                filePath.resize(wcslen(filePath.c_str()));
                 m_selectedFiles.push_back(filePath);
            }
        }
    }

    GlobalUnlock(stg.hGlobal);
    ReleaseStgMedium(&stg);

    return S_OK;
}


// IContextMenu
IFACEMETHODIMP CContextMenuExt::QueryContextMenu(const HMENU hmenu, const UINT indexMenu, const UINT idCmdFirst, UINT idCmdLast, const UINT uFlags) {

    m_idCmdFirst = idCmdFirst;//idk why we are doing this but it doesnt work without


    // when CMF_DEFAULTONLY flag is set, shouldnt add any items
    // check if files selected or bg click?
    if (uFlags & CMF_DEFAULTONLY || m_selectedFiles.empty()) {
        return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, 0);
    }

    UINT currentIdOffset = 0;

    // 1 main popup menu
    const HMENU hSubMenu = CreatePopupMenu();
    if (!hSubMenu) {
        return E_OUTOFMEMORY;
    }

    InsertMenuW(hSubMenu, currentIdOffset, MF_BYPOSITION | MF_STRING, MapCommandId(CMD_CHANGE_EXT, idCmdFirst), L"Change Extension");
    currentIdOffset++;
    InsertMenuW(hSubMenu, currentIdOffset, MF_BYPOSITION | MF_STRING, MapCommandId(CMD_UPLOAD_TMPFILES, idCmdFirst), L"Upload to TmpFiles");
    currentIdOffset++;
    InsertMenuW(hSubMenu, currentIdOffset, MF_BYPOSITION | MF_STRING, MapCommandId(CMD_COPY_PATH, idCmdFirst), L"Copy File Path(s)");
    currentIdOffset++;
    InsertMenuW(hSubMenu, currentIdOffset, MF_BYPOSITION | MF_STRING, MapCommandId(CMD_SHOW_HASHES, idCmdFirst), L"Show File Hashes");
    currentIdOffset++;
    
    // TODO: Add more items to hSubMenu here...
    


    // 2 submenu into the main context menu
    MENUITEMINFOW mii = { sizeof(mii) };
    mii.fMask = MIIM_SUBMENU | MIIM_STRING | MIIM_ID;
    mii.wID = idCmdFirst + currentIdOffset; // ID for the submenu itself. uses the next available offset
    mii.hSubMenu = hSubMenu;
    mii.dwTypeData = const_cast<LPWSTR>(L"Arti's Tools"); // CHANGE HERE FOR NAME

    if (!InsertMenuItemW(hmenu, indexMenu, TRUE, &mii)) {
         DestroyMenu(hSubMenu);
         return HRESULT_FROM_WIN32(GetLastError());
    }

    //currentIdOffset++; // ts wasted so much time, since it took some time to realize that menu itself needs inc (accounts for the submenu ID)

    // ret the number of cmd IDs used by cmds (excluding the submenu container)
    // if the highest command enum is N (e.g. CMD_SHOW_HASHES= 3), return N + 1
    // currentIdOffset tracks used IDs: one per command plus one for the submenu itself
    constexpr UINT maxCmdEnum = CMD_SHOW_HASHES;
    return MAKE_HRESULT(SEVERITY_SUCCESS, FACILITY_NULL, maxCmdEnum + 1);
}

IFACEMETHODIMP CContextMenuExt::InvokeCommand(const LPCMINVOKECOMMANDINFO pici) {
    // called by identifier (index) or verb string
    if (IS_INTRESOURCE(pici->lpVerb) == 0) {
        // maybe will add strings handlers in future idk
        return E_INVALIDARG;
    }
    
    // internal command ID
    // LOWORD(pici->lpVerb) is cmds zerobased offset relative to idCmdFirst
    UINT commandOffset = LOWORD(pici->lpVerb);
    
    switch (auto internalId = static_cast<CommandIds>(commandOffset)) {
        case CMD_CHANGE_EXT:
            // pici->hwnd as the parent window for dialogs
            Tools::SafelyChangeExtension(m_selectedFiles, pici->hwnd, g_hInstDll);
            return S_OK;

        case CMD_UPLOAD_TMPFILES:
            Tools::UploadToTmpFiles(m_selectedFiles, pici->hwnd);
            return S_OK;

        case CMD_COPY_PATH:
            Tools::CopyFilePathsToClipboard(m_selectedFiles, pici->hwnd);
            return S_OK;

        case CMD_SHOW_HASHES:
            Tools::ShowFileHashes(m_selectedFiles, pici->hwnd);
            return S_OK;

            // TODO: ADD MORE IF RERQUIRED

        default:
            // debug, show offset, useful when I fucked up currentIdOffset inc last time
            WCHAR szDebugMsg[100];
            StringCchPrintfW(szDebugMsg, ARRAYSIZE(szDebugMsg), L"Encountered an unknown cmd offset: %u", commandOffset);
            MessageBoxW(pici->hwnd, szDebugMsg, L"UNKNOWN_CMD_OFFSET", MB_OK | MB_ICONWARNING);
            return E_INVALIDARG;
    }
}


IFACEMETHODIMP CContextMenuExt::GetCommandString(UINT_PTR idCmd, const UINT uType, UINT* pReserved, const LPSTR pszName, const UINT cchMax) {
    // map absolute idCmd to internal 0-based CommandIds
    // idCmd is the cmds absolute ID
    if (idCmd < m_idCmdFirst) return E_INVALIDARG; // shouldnt happen if m_idCmdFirst is correct
    const auto internalId = static_cast<CommandIds>(idCmd - m_idCmdFirst);
    
    PCWSTR pszText = nullptr;

    // silly stuff, names require some docs
    switch (uType) {
        case GCS_HELPTEXTA:
        
        case GCS_HELPTEXTW:
            switch(internalId) {
                case CMD_CHANGE_EXT:
                    pszText = L" ";
                    break;
                case CMD_UPLOAD_TMPFILES:
                    pszText = L" ";
                    break;
                case CMD_COPY_PATH:
                    pszText = L" ";
                    break;
                case CMD_SHOW_HASHES:
                    pszText = L" ";
                    break;
            }
            break;

        case GCS_VERBA:
        
        case GCS_VERBW:
             switch(internalId) {
                case CMD_CHANGE_EXT:
                    pszText = L"MyTools_ChangeExt";
                    break;
                case CMD_UPLOAD_TMPFILES:
                    pszText = L"MyTools_UploadTmp";
                    break;
                case CMD_COPY_PATH:
                    pszText = L"MyTools_CopyPath";
                    break;
                case CMD_SHOW_HASHES:
                    pszText = L"MyTools_ShowHashes";
                    break;
            }
            break;

        // GCS_VALIDATE exists but often not needed for menus
    }

    if (!pszText) {
        return E_INVALIDARG;
    }

    if (uType & GCS_UNICODE) { // GCS_HELPTEXTW or GCS_VERBW
        StringCchCopyW(reinterpret_cast<PWSTR>(pszName), cchMax, pszText);
    } else { // GCS_HELPTEXTA or GCS_VERBA
        WideCharToMultiByte(CP_ACP, 0, pszText, -1, pszName, cchMax, nullptr, nullptr);
    }

    return S_OK;
}
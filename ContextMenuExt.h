#ifndef CONTEXTMENUEXT_H
#define CONTEXTMENUEXT_H
#pragma once
#include <windows.h>
#include <shlobj.h>
#include <vector>
#include <string>

class CContextMenuExt final : public IShellExtInit, public IContextMenu
{
    public:
        // IUnknown
        IFACEMETHODIMP QueryInterface(REFIID riid, void** ppv) override;
        IFACEMETHODIMP_(ULONG) AddRef() override;
        IFACEMETHODIMP_(ULONG) Release() override;

        // IShellExtInit
        IFACEMETHODIMP Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) override;

        // IContextMenu
        IFACEMETHODIMP QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
        IFACEMETHODIMP InvokeCommand(LPCMINVOKECOMMANDINFO pici) override;
        IFACEMETHODIMP GetCommandString(UINT_PTR idCmd, UINT uType, UINT* pReserved, LPSTR pszName, UINT cchMax) override;

        CContextMenuExt();

    protected:
        ~CContextMenuExt();

    private:

        UINT m_idCmdFirst = 0;

        
        long m_cRef;
        std::vector<std::wstring> m_selectedFiles;

        // command IDs relative to idCmdFirst
        enum CommandIds {
            CMD_INVALID = -1,
            CMD_CHANGE_EXT = 0,
            CMD_UPLOAD_TMPFILES = 1,
            // TODO: Add more when implemented
        };

        // mapz internal ID to actual command ID
        static UINT MapCommandId(const CommandIds internalId, const UINT idCmdFirst) {
            return idCmdFirst + static_cast<UINT>(internalId);
        }

};
#endif //CONTEXTMENUEXT_H

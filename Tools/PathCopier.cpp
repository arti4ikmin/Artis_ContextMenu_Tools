#include "../pch.h"
#include "PathCopier.h"
#include <algorithm>
#include <sstream>

#define MB MessageBoxW

// pasted from UploadToTmpFiles.cpp :money_mouth: (almost, updated a little will see which works better) // TODO: MAKE SHARED UTIL 
bool CopyToClipboardInternal(const HWND hwnd, const std::wstring& text) {
    if (!OpenClipboard(hwnd)) {
        MB(hwnd, L"Cant open the Clipboard (another app might be using it)", L"Clipboard Error", MB_OK | MB_ICONERROR);
        return false;
    }
    EmptyClipboard();
    const HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
    if (!hg) {
        CloseClipboard();
        MB(hwnd, L"Failed to alloc mem for clipboard", L"Clipboard Error", MB_OK | MB_ICONERROR);
        return false;
    }
    memcpy(GlobalLock(hg), text.c_str(), (text.length() + 1) * sizeof(wchar_t));
    GlobalUnlock(hg);
    if (!SetClipboardData(CF_UNICODETEXT, hg)) {
        GlobalFree(hg); // free if SetClipboardData failed
        CloseClipboard();
        MB(hwnd, L"Unable to set data to clipboard", L"Clipboard Error", MB_OK | MB_ICONERROR);
        return false;
    }
    // system now owns the memory pointed to by hg if SetClipboardData succeeded
    // no need for GlobalFree(hg) if SetClipboardData was successful
    CloseClipboard();
    return true;
}


void Tools::CopyFilePathsToClipboard(const std::vector<std::wstring>& files, HWND hwndParent) {
    if (files.empty()) {
        MB(hwndParent, L"No files selected to copy path.", L"Copy Path", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstringstream ss;
    for (size_t i = 0; i < files.size(); ++i) {
        std::wstring path = files[i];
        std::ranges::replace(path, L'\\', L'/');
        ss << path;
        if (i < files.size() - 1) {
            ss << L"\n";
        }
    }

    if (std::wstring pathsStr = ss.str(); CopyToClipboardInternal(hwndParent, pathsStr)) {
        MB(hwndParent, (L"File path(s) copied to clipboard:\n" + pathsStr).c_str(), L"Copy Path Success", MB_OK | MB_ICONINFORMATION);
    }
    else {
        // CopyToClipboardInternal
    }
}
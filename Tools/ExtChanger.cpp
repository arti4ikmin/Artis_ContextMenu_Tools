#include "../pch.h"
#include "ExtChanger.h"
#include "../resource.h"
#include <algorithm>
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include <string>

#define MB MessageBoxW

struct ConvertDialogParams {
    std::wstring currentFilePath;
    std::vector<std::wstring> availableTargetExtensions;
    std::wstring chosenTargetExtension;
    HINSTANCE hInstance;
};

std::vector<std::wstring> GetPossibleTargetExtensions(const std::wstring& sourceFileExtension) {
    std::wstring lowerSourceExt = sourceFileExtension;
    std::ranges::transform(lowerSourceExt, lowerSourceExt.begin(), towlower);

    const std::vector<std::wstring> commonImageTargets = {L".png", L".jpg", L".jpeg", L".bmp", L".tiff", L".webp"};
    std::vector<std::wstring> rawImageTargets = {L".png", L".jpg", L".jpeg", L".tiff"};

    if (lowerSourceExt == L".cr2" || lowerSourceExt == L".nef" || lowerSourceExt == L".arw") { // raw formats, seeing them first time for myself ngl
        return rawImageTargets;
    }
    if (lowerSourceExt == L".webp") {
        std::vector<std::wstring> targets = {L".png", L".jpg", L".jpeg", L".bmp", L".tiff"};
        return targets;
    }
    if (lowerSourceExt == L".tiff" || lowerSourceExt == L".tif") {
        std::vector<std::wstring> targets = {L".png", L".jpg", L".jpeg", L".bmp", L".webp"};
        targets.erase(std::ranges::remove_if(targets, [&](const std::wstring& t_ext){ return t_ext == L".tiff" || t_ext == L".tif"; }).begin(), targets.end());
        return targets;
    }
    if (lowerSourceExt == L".svg") {
        return {L".png"};
    }
    if (lowerSourceExt == L".jpg" || lowerSourceExt == L".jpeg" || 
        lowerSourceExt == L".png" || lowerSourceExt == L".bmp" ||
        lowerSourceExt == L".gif") {
        std::vector<std::wstring> targets = commonImageTargets;
        // del self from targets
        targets.erase(std::ranges::remove_if(targets,[&](const std::wstring& t_ext){ return t_ext == lowerSourceExt; }).begin(), targets.end());
        if (lowerSourceExt == L".jpg" || lowerSourceExt == L".jpeg") {
            targets.erase(std::ranges::remove_if(targets, [&](const std::wstring& t_ext){ return (lowerSourceExt == L".jpg" && t_ext == L".jpeg") || (lowerSourceExt == L".jpeg" && t_ext == L".jpg"); }).begin(), targets.end());
        }
        return targets;
    }

    //more to go, for now if unknown return empty
    return {};
}

INT_PTR CALLBACK ConvertDialogProc(const HWND hDlg, const UINT message, const WPARAM wParam, const LPARAM lParam) {
    ConvertDialogParams* pParams = nullptr;

    if (message == WM_INITDIALOG) {
        pParams = reinterpret_cast<ConvertDialogParams*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, reinterpret_cast<LONG_PTR>(pParams));
    } else {
        pParams = reinterpret_cast<ConvertDialogParams*>(GetWindowLongPtr(hDlg, DWLP_USER));
    }

    if (!pParams && message != WM_INITDIALOG) { return FALSE; } // pParams may be null before WM_INITDIALOG storage -- as I understood the docs heavily mention that idk myself tbh

    switch (message) {
        case WM_INITDIALOG:
        {
            // pParams is valid from the reinterpret_cast
            const std::wstring fileNameOnly = PathFindFileNameW(pParams->currentFilePath.c_str());
            SetDlgItemTextW(hDlg, IDC_FILENAME_LABEL, fileNameOnly.c_str());

            if (const HWND hCombo = GetDlgItem(hDlg, IDC_TARGET_EXT_COMBO)) {
                for (const auto& ext : pParams->availableTargetExtensions) {
                    SendMessageW(hCombo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(ext.c_str()));
                }
                if (!pParams->availableTargetExtensions.empty()) {
                    SendMessageW(hCombo, CB_SETCURSEL, 0, 0);
                } else {
                    EnableWindow(hCombo, FALSE);
                    EnableWindow(GetDlgItem(hDlg, IDOK), FALSE); // grey out OK if no options
                }
            }

            RECT rcOwner, rcDlg, rc;
            GetWindowRect(GetParent(hDlg), &rcOwner); // parent window (explorer/desktop)
            GetWindowRect(hDlg, &rcDlg);
            CopyRect(&rc, &rcOwner);
            OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
            OffsetRect(&rc, -rc.left, -rc.top);
            OffsetRect(&rc, -rcDlg.right / 2, -rcDlg.bottom / 2);
            SetWindowPos(hDlg, HWND_TOP, rcOwner.left + rc.right / 2, rcOwner.top + rc.bottom / 2, 0, 0, SWP_NOSIZE);

            return TRUE;
        }

        case WM_COMMAND:
            {
                if (LOWORD(wParam) == IDOK) {
                    if (pParams) {
                        const HWND hCombo = GetDlgItem(hDlg, IDC_TARGET_EXT_COMBO);
                        if (const int selectedIndex = SendMessage(hCombo, CB_GETCURSEL, 0, 0); selectedIndex != CB_ERR) {
                            WCHAR buffer[32];
                            SendMessageW(hCombo, CB_GETLBTEXT, selectedIndex, reinterpret_cast<LPARAM>(buffer));
                            pParams->chosenTargetExtension = buffer;
                            EndDialog(hDlg, IDOK);
                        } else {
                            MB(hDlg, L"Please select a target extension.", L"Selection Missing", MB_OK | MB_ICONWARNING);
                        }
                    }
                    return TRUE;
                }
                if (LOWORD(wParam) == IDCANCEL) {
                    EndDialog(hDlg, IDCANCEL);
                    return TRUE;
                }
            }
        break;
    }
    return FALSE;
}

void Tools::SafelyChangeExtension(const std::vector<std::wstring>& files, HWND hwndParent, HINSTANCE hInstDll) {
    if (files.empty()) {
        MB(hwndParent, L"No files selected for extension change", L"Artis Tools", MB_OK | MB_ICONINFORMATION);
        return;
    }

    for (const auto& filePath : files) {
        LPCWSTR pszExt = PathFindExtensionW(filePath.c_str());
        if (!pszExt || *pszExt==L'\0') {
            MB(hwndParent, (std::wstring(L"Skipping: ") + PathFindFileNameW(filePath.c_str()) + L" (no extension)").c_str(), L"Info", MB_OK | MB_ICONINFORMATION);
            continue;
        }
        std::wstring currentExt = pszExt;

        ConvertDialogParams params{filePath, GetPossibleTargetExtensions(currentExt), L"", hInstDll};
        if (params.availableTargetExtensions.empty()) {
            MB(hwndParent, (L"No conversion options for '"+currentExt+L"' files").c_str(), L"Conversion Info", MB_OK | MB_ICONINFORMATION);
            continue;
        }

        if (INT_PTR res = DialogBoxParamW(hInstDll, reinterpret_cast<LPCWSTR>(MAKEINTRESOURCE(IDD_CONVERT_DIALOG)), hwndParent, ConvertDialogProc, reinterpret_cast<LPARAM>(&params)); res!=IDOK || params.chosenTargetExtension.empty()) continue;

        std::wstring base = filePath.substr(0, filePath.size()-currentExt.size());
        std::wstring newPath = base + params.chosenTargetExtension;

        try { // fuck opencv for failing that much, trycatch is sadly a must here
            auto input = cv::String(filePath.begin(), filePath.end());
            cv::Mat img = cv::imread(input, cv::IMREAD_UNCHANGED);
            if (img.empty()) {
                throw std::runtime_error("Failed to load img");
            }
            std::vector<int> paramsCV;
            // formatspecific params if needed sometime
            if (params.chosenTargetExtension==L".jpg"||params.chosenTargetExtension==L".jpeg") paramsCV = {cv::IMWRITE_JPEG_QUALITY, 95};
            else if (params.chosenTargetExtension==L".png") paramsCV = {cv::IMWRITE_PNG_COMPRESSION, 3};

            if (auto output = cv::String(newPath.begin(), newPath.end()); !cv::imwrite(output, img, paramsCV)) {
                throw std::runtime_error("Failed to write img");
            }
            //MB(hwndParent, (std::wstring(L"Converted: ") + PathFindFileNameW(filePath.c_str()) + L" -> " + PathFindFileNameW(newPath.c_str())).c_str(), L"Conversion Success", MB_OK | MB_ICONINFORMATION);
        }
        catch (const std::exception& ex) {

            MB(hwndParent,
                        (L"Error converting '" + std::wstring(PathFindFileNameW(filePath.c_str())) + L"': " +
                            [](const std::string& s)
                            {
                                const int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
                                std::wstring ws(len, 0);
                                MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &ws[0], len);
                                ws.pop_back();
                                return ws;
                            }(ex.what())).c_str(),
                        L"Conversion Error", MB_OK | MB_ICONERROR);

            //funny enough cmake was fucking w me about deprecation of wstring_convert<std::codecvt_utf8<wchar_t>>() so I had to use the workarounf provided above, what a ******
            //MB(hwndParent, (std::wstring(L"Error converting '") + PathFindFileNameW(filePath.c_str()) + L"': " + std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(ex.what())).c_str(), L"Conversion Error", MB_OK | MB_ICONERROR);
        }
    }
}
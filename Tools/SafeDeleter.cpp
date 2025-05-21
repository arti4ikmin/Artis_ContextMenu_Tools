#include "../pch.h"
#include "SafeDeleter.h"

#include <random>
#include <sstream>

#define MB MessageBoxW


// SecureOverwriteAndDeleteInternal
static bool SOaDI(const std::wstring& filePath, std::wstring& errMsg) {
    DWORD origAttr = GetFileAttributesW(filePath.c_str());
    if (origAttr != INVALID_FILE_ATTRIBUTES && (origAttr & FILE_ATTRIBUTE_READONLY)) {
        if (!SetFileAttributesW(filePath.c_str(), origAttr & ~FILE_ATTRIBUTE_READONLY)) {
            errMsg = L"Couldnt remove readonly attribute";
            return false;
        }
    }

    const HANDLE hFile = CreateFileW(
        filePath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
        nullptr
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        if (DWORD dwError = GetLastError(); dwError == ERROR_SHARING_VIOLATION) {
            errMsg = L"File is in use by another process";
        }
        else if (dwError == ERROR_ACCESS_DENIED) {
            errMsg = L"Access denied, check perms or if file is readonly and couldnt be changed";
        }
        else {
            errMsg = L"Failed to open file (Error: " + std::to_wstring(dwError) + L")";
        }
        return false;
    }

    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        errMsg = L"Failed to get file size";
        return false;
    }


    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution dist(0, 255);
    

    constexpr DWORD buffSize = 65536;
    std::vector<BYTE> buffer(buffSize);
    
    
    if (SetFilePointerEx(hFile, {}, nullptr, FILE_BEGIN) == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR) {
         CloseHandle(hFile);
         errMsg = L"Failed to get to beginning of file";
         return false;
    }


    for (int pass = 0; pass < 3; ++pass) { // 3 is the number of iterations to make (multipass)
        if (!SetFilePointerEx(hFile, {}, nullptr, FILE_BEGIN)) {
            CloseHandle(hFile);
            errMsg = L"Failed to get to beginnign of file for overwrite pass";
            return false;
        }

        LONGLONG bytesRemaining = fileSize.QuadPart;
        while (bytesRemaining > 0) {
            DWORD bytesToWrite = (bytesRemaining < static_cast<LONGLONG>(buffSize)) ? static_cast<DWORD>(bytesRemaining) : buffSize;
            // rand data
            for (DWORD i = 0; i < bytesToWrite; ++i) {
                buffer[i] = static_cast<BYTE>(dist(rng));
            }
            

            DWORD bytesWritten;
            if (!WriteFile(hFile, buffer.data(), bytesToWrite, &bytesWritten, nullptr) || bytesWritten != bytesToWrite) {
                CloseHandle(hFile);
                errMsg = L"Failed to write overwrite data to file (pass " + std::to_wstring(pass + 1) + L")";
                return false;
            }

            bytesRemaining -= bytesWritten;
        }
    }

    
    if (!FlushFileBuffers(hFile)) {
        CloseHandle(hFile);
        errMsg = L"Failed to flush file buffers after overwriting";
        return false;
    }
    

    CloseHandle(hFile);

    if (!DeleteFileW(filePath.c_str())) {
        DWORD dwError = GetLastError();
        errMsg = L"Failed to delete file after overwriting (Error: " + std::to_wstring(dwError) + L").";
        // if (originalAttributes != INVALID_FILE_ATTRIBUTES && (originalAttributes & FILE_ATTRIBUTE_READONLY)) {
        //     SetFileAttributesW(filePath.c_str(), originalAttributes);
        // } // TODO: TEST IF TS WORKS, IMPLEMENT (RESTORE READONLY)
        return false;
    }

    return true;
}

void Tools::SafeDelFiles(const std::vector<std::wstring>& files, HWND hwndParent) {
    if (files.empty()) {
        MB(hwndParent, L"No files selected for safe delete", L"Safe Delete", MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::wstring confirmMsg;
    if (files.size() == 1) {
        // no dir del yet
        if (const DWORD attr = GetFileAttributesW(files[0].c_str()); attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)) {
             MB(hwndParent, (L"Cant delete '" + std::wstring(PathFindFileNameW(files[0].c_str())) + L"'.\nSafe delete for dirs isnt yet supported").c_str(), L"Safe Del Not Supported", MB_OK | MB_ICONWARNING);
             return;
        }
        confirmMsg = L"Are you sure you want to securely delete '" + std::wstring(PathFindFileNameW(files[0].c_str())) + L"'?\n\nThis overwrites the file with random data before deletion and CANNOT BE UNDONE";
    }
    else {
        confirmMsg = L"Are you sure you want to securely delete " + std::to_wstring(files.size()) + L" selected item(s)?\nFiles will be overwritten with random data before deletion\nDirectories will be skipped\n\nThis action CANNOT BE UNDONE";
    }

    if (MB(hwndParent, confirmMsg.c_str(), L"Confirm Safe Delete", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
        return;
    }

    int delCount = 0;
    std::vector<std::wstring> failedItems;
    std::vector<std::wstring> skippedDirs;

    for (const auto& filePath : files) {
        const DWORD fA = GetFileAttributesW(filePath.c_str());

        if (fA == INVALID_FILE_ATTRIBUTES) {
            failedItems.push_back(std::wstring(PathFindFileNameW(filePath.c_str())) + L" (could not get attributes)");
            continue;
        }

        if (fA & FILE_ATTRIBUTE_DIRECTORY) {
            skippedDirs.emplace_back(PathFindFileNameW(filePath.c_str()));
            continue;
        }

        if (std::wstring errorMsgForThisFile; SOaDI(filePath, errorMsgForThisFile)) {
            delCount++;
        } else {
            failedItems.push_back(std::wstring(PathFindFileNameW(filePath.c_str())) + L": " + errorMsgForThisFile);
        }
    }
    
    std::wstringstream sumMsg;
    sumMsg << L"Safe Delete Operation Complete\n\n";
    if (delCount > 0) {
        sumMsg << L"Successfully deleted: " << delCount << L" file(s)\n";
    }
    if (!skippedDirs.empty()) {
        sumMsg << L"Skipped " << skippedDirs.size() << L" director(y/ies) (not supported):\n";
        for (size_t i = 0; i < skippedDirs.size(); ++i) {
            sumMsg << L"- " << skippedDirs[i] << (i == skippedDirs.size() -1 ? L"" : L"\n");
        }
        sumMsg << L"\n";
    }
    if (!failedItems.empty()) {
        sumMsg << L"Failed to delete " << failedItems.size() << L" item(s):\n";
        for (size_t i = 0; i < failedItems.size(); ++i) {
            sumMsg << L"- " << failedItems[i] << (i == failedItems.size() -1 ? L"" : L"\n");
        }
    }
    if (delCount == 0 && failedItems.empty() && skippedDirs.empty() && !files.empty()) {
         sumMsg << L"No eligible files were processed\n";
    }


    MB(hwndParent, sumMsg.str().c_str(), L"Safe Delete Summary", MB_OK | (failedItems.empty() ? MB_ICONINFORMATION : MB_ICONWARNING));
}

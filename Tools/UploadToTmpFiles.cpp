#include "../pch.h"
#include "UploadToTmpFiles.h"
#include <wininet.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <random>

constexpr long long MAX_FILE_SIZE_BYTES = 104857600; // 100mb

#define MB MessageBoxW

bool CopyToClipboard(const HWND hwnd, const std::wstring& text) {
    if (!OpenClipboard(hwnd)) {
        return false;
    }
    EmptyClipboard();
    const HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, (text.length() + 1) * sizeof(wchar_t));
    if (!hg) {
        CloseClipboard();
        return false;
    }
    memcpy(GlobalLock(hg), text.c_str(), (text.length() + 1) * sizeof(wchar_t));
    GlobalUnlock(hg);
    SetClipboardData(CF_UNICODETEXT, hg);
    CloseClipboard();
    GlobalFree(hg); // simple but scary, """The system now owns the memory, but if SetClipboardData fails, we should free. For simplicity, we'll assume success or the system handles it. A more robust approach checks SetClipboardData's return """
    return true;
}

// multipart boundary
std::string GenerateBoundary() {
    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<unsigned int> dist;
    std::stringstream ss;
    ss << "----WebKitFormBoundary";
    for (int i = 0; i < 16; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (dist(rng) % 256);
    }
    return ss.str();
}


void Tools::UploadToTmpFiles(const std::vector<std::wstring>& files, HWND hwndParent) {
    if (files.size() != 1) {
        MB(hwndParent, L"Please select only one file to upload, archiving might be coming in future", L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }

    const std::wstring& filePath = files[0];

    // size
    HANDLE hFile = CreateFileW(filePath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        MB(hwndParent, (L"Failed to open file: " + filePath).c_str(), L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }
    LARGE_INTEGER fileSize;
    if (!GetFileSizeEx(hFile, &fileSize)) {
        CloseHandle(hFile);
        MB(hwndParent, (L"Failed to get file size: " + filePath).c_str(), L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }
    CloseHandle(hFile);

    if (fileSize.QuadPart == 0) {
        MB(hwndParent, L"Selected file is empty and cant be uploaded", L"TmpFiles Upload Error", MB_OK | MB_ICONWARNING);
        return;
    }
    if (fileSize.QuadPart > MAX_FILE_SIZE_BYTES) {
        MB(hwndParent, L"File is larger than 100MB and cant be uploaded (service limits)", L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }


    std::ifstream fileStream(filePath, std::ios::binary | std::ios::ate);
    if (!fileStream.is_open()) {
        MB(hwndParent, (L"(How could that happen???)\nFailed to read file content: " + filePath).c_str(), L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }
    std::streamsize size = fileStream.tellg();
    fileStream.seekg(0, std::ios::beg);
    std::vector<char> fileBuffer(size);
    if (!fileStream.read(fileBuffer.data(), size)) {
        MB(hwndParent, (L"Error reading file into buffer: " + filePath).c_str(), L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }
    fileStream.close();


    // WinINet :pray:
    HINTERNET hInternet = InternetOpenW(L"ArtiContextMenuUploader/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
    if (!hInternet) {
        MB(hwndParent, L"InternetOpenW failed", L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }

    HINTERNET hConnect = InternetConnectW(hInternet, L"tmpfiles.org", INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, 0, 0);
    if (!hConnect) {
        InternetCloseHandle(hInternet);
        MB(hwndParent, L"InternetConnectW failed", L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }

    LPCWSTR rgpszAcceptTypes[] = {L"*/*", nullptr}; // any response
    HINTERNET hRequest = HttpOpenRequestW(hConnect, L"POST", L"/api/v1/upload", nullptr, nullptr, rgpszAcceptTypes, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, 0);
    if (!hRequest) {
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        MB(hwndParent, L"HttpOpenRequestW failed", L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }

    //
    std::string boundary = GenerateBoundary();
    std::string contentTypeHeader = "Content-Type: multipart/form-data; boundary=" + boundary;

    std::ostringstream requestBodyStream;
    // part for the file
    requestBodyStream << "--" << boundary << "\r\n";
    // WCHAR filename to char for header
    char mbFilename[MAX_PATH];
    WideCharToMultiByte(CP_UTF8, 0, PathFindFileNameW(filePath.c_str()), -1, mbFilename, MAX_PATH, nullptr, nullptr);
    requestBodyStream << "Content-Disposition: form-data; name=\"file\"; filename=\"" << mbFilename << "\"\r\n";
    requestBodyStream << "Content-Type: application/octet-stream\r\n\r\n"; // so called mime type :shrug:


    std::string preData = requestBodyStream.str();
    requestBodyStream.str(""); // safety

    std::string postData = "\r\n--" + boundary + "--\r\n";

    DWORD totalLength = preData.length() + fileBuffer.size() + postData.length();

    HttpAddRequestHeadersA(hRequest, contentTypeHeader.c_str(), -1L, HTTP_ADDREQ_FLAG_ADD);


    // ACTUAL SEND HERE
    INTERNET_BUFFERSA BufferIn;
    BufferIn.dwStructSize = sizeof(INTERNET_BUFFERSA);
    BufferIn.Next = nullptr;
    BufferIn.lpcszHeader = nullptr;
    BufferIn.dwHeadersLength = 0;
    BufferIn.dwHeadersTotal = 0;
    BufferIn.lpvBuffer = nullptr;
    BufferIn.dwBufferLength = 0;
    BufferIn.dwBufferTotal = totalLength;
    BufferIn.dwOffsetLow = 0;
    BufferIn.dwOffsetHigh = 0;

    if (!HttpSendRequestExA(hRequest, &BufferIn, nullptr, HSR_INITIATE, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        MB(hwndParent, L"HttpSendRequestEx failed", L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
        return;
    }

    DWORD dwBytesWritten;
    if (!InternetWriteFile(hRequest, preData.c_str(), preData.length(), &dwBytesWritten) || dwBytesWritten != preData.length()) {
        MB(hwndParent, L"preData failed", L"Upload Error", MB_OK | MB_ICONERROR);
    }
    if (!InternetWriteFile(hRequest, fileBuffer.data(), fileBuffer.size(), &dwBytesWritten) || dwBytesWritten != fileBuffer.size()) {
        MB(hwndParent, L"fileBuffer failed", L"Upload Error", MB_OK | MB_ICONERROR);
    }
    if (!InternetWriteFile(hRequest, postData.c_str(), postData.length(), &dwBytesWritten) || dwBytesWritten != postData.length()) {
        MB(hwndParent, L"postData failed", L"Upload Error", MB_OK | MB_ICONERROR);
    }

    if (!HttpEndRequestW(hRequest, nullptr, 0, 0)) {
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        MB(hwndParent, L"HttpEndRequest failed", L"Upload Error", MB_OK | MB_ICONERROR);
        return;
    }

    // RESPONSE
    std::string responseString;
    char responseBuffer[4096];
    DWORD dwBytesRead;
    while (InternetReadFile(hRequest, responseBuffer, sizeof(responseBuffer) -1, &dwBytesRead) && dwBytesRead > 0) {
        responseBuffer[dwBytesRead] = '\0';
        responseString.append(responseBuffer);
    }
    
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    if (responseString.empty()) {
         MB(hwndParent, L"No response from server (???)", L"TmpFiles Upload Error", MB_OK | MB_ICONERROR);
         return;
    }

    // tryin to avoid json packages to reduce dll count in the end :money_mouth:
    if (responseString.find("\"status\":\"success\"") != std::string::npos) {
        const std::string urlKey = "\"url\":\"";
        if (size_t urlPos = responseString.find(urlKey); urlPos != std::string::npos) {
            urlPos += urlKey.length();
            if (size_t endQuote = responseString.find('\"', urlPos); endQuote != std::string::npos) {
                std::string url_str = responseString.substr(urlPos, endQuote - urlPos);

                if (std::wstring url_wstr(url_str.begin(), url_str.end()); CopyToClipboard(hwndParent, url_wstr)) {
                    std::wstring successMsg = L"File uploaded successfully!\nLink copied to clipboard:\n" + url_wstr;
                    MB(hwndParent, successMsg.c_str(), L"TmpFiles Upload Success", MB_OK | MB_ICONINFORMATION);
                } else {
                    std::wstring partialSuccessMsg = L"File uploaded successfully!\nLink:\n" + url_wstr + L"\n\n(Failed to copy to clipboard)";
                    MB(hwndParent, partialSuccessMsg.c_str(), L"TmpFiles Upload Success", MB_OK | MB_ICONWARNING);
                }
            }
        }
    } else {
        std::wstring errMsg(L"Upload failed Server response:\n");
        errMsg += std::wstring(responseString.begin(), responseString.end());
        MB(hwndParent, errMsg.c_str(), L"Fatal Upload Fail", MB_OK | MB_ICONERROR);
    }

}
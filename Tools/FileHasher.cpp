#include "../pch.h"
#include "FileHasher.h"

#include <bcrypt.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ntstatus.h>

#define MB MessageBoxW

std::wstring BytesToHexString(const BYTE* data, DWORD size) {
    std::wostringstream ss;
    ss << std::hex << std::setfill(L'0');
    for (DWORD i = 0; i < size; ++i) {
        ss << std::setw(2) << data[i];
    }
    return ss.str();
}

std::wstring CalcFileHash(const std::wstring& filePath, const LPCWSTR pszAlgId, std::wstring& errorMessage) {
    errorMessage.clear();
    BCRYPT_ALG_HANDLE hAlg = nullptr;
    BCRYPT_HASH_HANDLE hHash = nullptr;
    NTSTATUS status = STATUS_UNSUCCESSFUL;
    DWORD cbHashObject = 0, cbResult = 0;
    DWORD cbHash = 0;
    std::vector<BYTE> pbHashObject;
    std::vector<BYTE> pbHash;

    status = BCryptOpenAlgorithmProvider(&hAlg, pszAlgId, nullptr, 0);
    if (!BCRYPT_SUCCESS(status)) {
        errorMessage = L"BCryptOpenAlgorithmProvider failed";
        goto cleanup;
    }

    status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PBYTE>(&cbHashObject), sizeof(DWORD), &cbResult, 0);
    if (!BCRYPT_SUCCESS(status)) {
        errorMessage = L"BCryptGetProperty(BCRYPT_OBJECT_LENGTH) failed";
        goto cleanup;
    }
    pbHashObject.resize(cbHashObject);

    status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, reinterpret_cast<PBYTE>(&cbHash), sizeof(DWORD), &cbResult, 0);
    if (!BCRYPT_SUCCESS(status)) {
        errorMessage = L"BCryptGetProperty(BCRYPT_HASH_LENGTH) failed";
        goto cleanup;
    }
    pbHash.resize(cbHash);

    status = BCryptCreateHash(hAlg, &hHash, pbHashObject.data(), cbHashObject, nullptr, 0, 0);
    if (!BCRYPT_SUCCESS(status)) {
        errorMessage = L"BCryptCreateHash failed";
        goto cleanup;
    }

    {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            errorMessage = L"Failed to open file: " + filePath;
            goto cleanup;
        }

        char buffer[4096];
        while (file.good()) {
            file.read(buffer, sizeof(buffer));
            std::streamsize bytesRead = file.gcount();
            if (bytesRead > 0) {
                status = BCryptHashData(hHash, reinterpret_cast<PBYTE>(buffer), static_cast<ULONG>(bytesRead), 0);
                if (!BCRYPT_SUCCESS(status)) {
                    errorMessage = L"BCryptHashData failed";
                    goto cleanup;
                }
            }
        }
    }


    status = BCryptFinishHash(hHash, pbHash.data(), cbHash, 0);
    if (!BCRYPT_SUCCESS(status)) {
        errorMessage = L"BCryptFinishHash failed";
        goto cleanup;
    }

    return BytesToHexString(pbHash.data(), cbHash);

    cleanup:
        if (hHash) BCryptDestroyHash(hHash);
        if (hAlg) BCryptCloseAlgorithmProvider(hAlg, 0);
        return L"";
}


void Tools::ShowFileHashes(const std::vector<std::wstring>& files, HWND hwndParent) {
    if (files.empty()) {
        MB(hwndParent, L"No file selected", L"File Hashes", MB_OK | MB_ICONINFORMATION);
        return;
    }
    if (files.size() > 1) {
        MB(hwndParent, L"Please select only one file to calc hashes", L"File Hashes", MB_OK | MB_ICONWARNING);
        return;
    }

    const std::wstring& filePath = files[0];
    std::wstring errorMsg;

    const std::wstring md5Hash = CalcFileHash(filePath, BCRYPT_MD5_ALGORITHM, errorMsg);
    if (!errorMsg.empty()) {
        MB(hwndParent, (L"MD5 calc error: " + errorMsg).c_str(), L"Hashing Error", MB_OK | MB_ICONERROR);
        return;
    }
    const std::wstring sha256Hash = CalcFileHash(filePath, BCRYPT_SHA256_ALGORITHM, errorMsg);
    if (!errorMsg.empty()) {
        MB(hwndParent, (L"SHA256 calc error: " + errorMsg).c_str(), L"Hashing Error", MB_OK | MB_ICONERROR);
        return;
    }

    std::wostringstream result;
    result << L"File: " << PathFindFileNameW(filePath.c_str()) << L"\n\n"
           << L"MD5: " << md5Hash << L"\n\n"
           << L"SHA256: " << sha256Hash;

    MB(hwndParent, result.str().c_str(), L"File Hashes", MB_OK | MB_ICONINFORMATION);
}
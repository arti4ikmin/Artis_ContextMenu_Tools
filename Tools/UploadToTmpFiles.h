#ifndef NETWORKUTILS_H
#define NETWORKUTILS_H

#pragma once
#include <vector>
#include <string>
#include <windows.h>

namespace Tools {
    void UploadToTmpFiles(const std::vector<std::wstring>& files, HWND hwndParent);
}

#endif // NETWORKUTILS_H
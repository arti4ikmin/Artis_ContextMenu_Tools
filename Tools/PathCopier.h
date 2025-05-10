#ifndef PATHCOPIER_H
#define PATHCOPIER_H

#pragma once
#include <vector>
#include <string>
#include <windows.h>

namespace Tools {
    void CopyFilePathsToClipboard(const std::vector<std::wstring>& files, HWND hwndParent);
}

#endif // PATHCOPIER_H
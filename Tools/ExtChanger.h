#ifndef EXTCHANGER_H
#define EXTCHANGER_H

#pragma once
#include <vector>
#include <string>
#include <windows.h>

namespace Tools {
    void SafelyChangeExtension(const std::vector<std::wstring>& files, HWND hwndParent, HINSTANCE hInstDll);
}

#endif // EXTCHANGER_H
#ifndef SAFEDELETER_H
#define SAFEDELETER_H

#pragma once
#include <vector>
#include <string>
#include <windows.h>

namespace Tools {
    void SafeDelFiles(const std::vector<std::wstring>& files, HWND hwndParent);
}

#endif // SAFEDELETER_H
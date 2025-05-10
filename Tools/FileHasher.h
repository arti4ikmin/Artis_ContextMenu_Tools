#ifndef FILEHASHER_H
#define FILEHASHER_H

#pragma once
#include <vector>
#include <string>

namespace Tools {
    void ShowFileHashes(const std::vector<std::wstring>& files, HWND hwndParent);
}

#endif // FILEHASHER_H
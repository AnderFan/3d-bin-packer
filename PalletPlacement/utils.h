#pragma once
#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <string>
#include <windows.h>

// Helper function to convert UTF-8 to Wide string
inline std::wstring utf8_to_wstring(const char* utf8) {
    if (!utf8) return L"";
    
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    if (size == 0) return L"";
    
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8, -1, &result[0], size);
    return result;
}

#endif // UTILS_H_INCLUDED

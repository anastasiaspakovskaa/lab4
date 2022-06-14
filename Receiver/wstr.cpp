#include "wstr.h"

std::wstring convertToWideString(const std::string& str) {
    const char* c_str = str.c_str();
    wchar_t* wstr = new wchar_t[str.length() + 1];
    mbstowcs(wstr, c_str, str.size() + 1);
    std::wstring wide(wstr);
    delete[] wstr;
    return wide;
}
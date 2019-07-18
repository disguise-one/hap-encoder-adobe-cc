#include "string_conversion.hpp"

#ifndef _WIN64
#include <codecvt>
#else
#include <Windows.h>
#endif

#include <locale>

std::wstring to_wstring(const std::string& str)
{
#ifdef _WIN64
    const int n_chars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring buffer(n_chars-1, ' ');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], n_chars);
    return buffer;
#else
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(str);
#endif
}

std::string to_string(const std::wstring& str)
{
#ifdef _WIN64
    const int n_chars = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string buffer(n_chars-1, ' ');
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &buffer[0], n_chars, nullptr, nullptr);
    return buffer;
#else
    //setup converter
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

    return converter.to_bytes(str);
#endif
}


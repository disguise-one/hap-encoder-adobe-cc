#include "string_conversion.hpp"

#ifndef _WIN64
#include <codecvt>
#include <CoreFoundation/CFString.h>
#include <vector>
#else
#include <Windows.h>
#endif

#include <locale>

static inline int aUTF16CharLength(const A_UTF16Char* inStr)
{
    int ret = 0;
    if (inStr)
    {
        for (;*inStr; ++inStr, ++ret)
        {}
    }
    return ret;
}

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

std::wstring to_wstring(const A_UTF16Char *str)
{
    return to_wstring(to_string(str));
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

std::string to_string(const A_UTF16Char *str)
{
#ifdef _WIN64
    return to_string(std::wstring(reinterpret_cast<const wchar_t *>(str)));
#else
    CFIndex bytes = aUTF16CharLength(str) * sizeof(A_UTF16Char);
    CFStringRef input = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(str), bytes, kCFStringEncodingUTF16, false, kCFAllocatorNull);
    const char *output = CFStringGetCStringPtr(input, kCFStringEncodingUTF8);
    if (!output)
    {
        CFIndex maximum = CFStringGetMaximumSizeForEncoding(CFStringGetLength(input), kCFStringEncodingUTF8);
        std::vector<char> buffer(maximum + 1);
        Boolean result = CFStringGetCString(input, buffer.data(), buffer.size(), kCFStringEncodingUTF8);
        if (result)
        {
            output = buffer.data();
        }
    }

    std::string final;
    if (output)
    {
        final = output;
    }

    CFRelease(input);

    return final;
#endif
}
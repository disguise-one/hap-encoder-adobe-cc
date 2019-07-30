#include "string_conversion.hpp"

#ifdef __APPLE__
#include <codecvt>
#include <CoreFoundation/CFString.h>
#include <vector>
#else
#include <Windows.h>
#endif

#include <locale>

#ifdef __APPLE__
template <class T>
static inline int aUTF16CharLength(const T* inStr)
{
    int ret = 0;
    if (inStr)
    {
        for (;*inStr; ++inStr, ++ret)
        {}
    }
    return ret;
}
#endif

std::wstring SDKStringConvert::to_wstring(const std::string& str)
{
#ifdef _WIN32
    const int n_chars = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring buffer(n_chars-1, ' ');
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], n_chars);
    return buffer;
#else
    return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>{}.from_bytes(str);
#endif
}

std::wstring SDKStringConvert::to_wstring(const uint16_based_type *str)
{
#ifdef _WIN32
	return std::wstring(reinterpret_cast<const wchar_t*>(str));
#else
	return to_wstring(to_string(str));
#endif
}

std::string SDKStringConvert::to_string(const std::wstring& str)
{
#ifdef _WIN32
    const int n_chars = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string buffer(static_cast<size_t>(n_chars) - 1, ' ');
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &buffer[0], n_chars, nullptr, nullptr);
    return buffer;
#else
    //setup converter
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;

    return converter.to_bytes(str);
#endif
}

#ifdef __APPLE__
static bool cf_to_buffer(CFStringRef input, CFStringEncoding encoding, UInt8 *dst, CFIndex maxBuffLen)
{
    CFRange	range = { 0, 0 };
	range.length = CFStringGetLength(input);
    CFIndex copied;
	CFStringGetBytes(input, range, encoding, 0, false, dst, maxBuffLen, &copied);
	dst[copied] = 0;
    if (copied != 0)
    {
        return true;
    }
    return false;
}
#endif

std::string SDKStringConvert::to_string(const uint16_based_type *str)
{
#ifdef _WIN32
    return to_string(to_wstring(str));
#else
    CFIndex bytes = aUTF16CharLength(str) * sizeof(uint16_based_type);
    CFStringRef input = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(str), bytes, kCFStringEncodingUTF16, false, kCFAllocatorNull);
    const char *output = CFStringGetCStringPtr(input, kCFStringEncodingUTF8);
    if (!output)
    {
        CFIndex maximum = CFStringGetMaximumSizeForEncoding(CFStringGetLength(input), kCFStringEncodingUTF8);
        std::vector<char> buffer(maximum + 1);
        if (cf_to_buffer(input, kCFStringEncodingUTF8, reinterpret_cast<UInt8 *>(buffer.data()), buffer.size()))
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

#ifdef _WIN32
void SDKStringConvert::to_buffer(const std::string& str, wchar_t* dst, size_t dstSizeInChars)
{
	to_buffer(to_wstring(str), dst, dstSizeInChars);
}
#else
void SDKStringConvert::to_buffer(const std::string& str, uint16_based_type* dst, size_t dstChars)
{
	to_buffer(to_wstring(str), dst, dstChars);
}
#endif

#ifdef _WIN32
void SDKStringConvert::to_buffer(const std::wstring& str, wchar_t* dst, size_t dstChars)
{
	wcscpy_s(dst, dstChars, str.c_str());
}
#else
void SDKStringConvert::to_buffer(const std::wstring& str, uint16_based_type* dst, size_t dstChars)
{
    CFIndex bytes = str.length() * sizeof(wchar_t);
	CFStringRef input = CFStringCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8 *>(str.c_str()), bytes, kCFStringEncodingUTF32LE, false, kCFAllocatorNull);
    cf_to_buffer(input, kCFStringEncodingUTF16, reinterpret_cast<UInt8 *>(dst), dstChars * (sizeof(uint16_based_type)));
	CFRelease(input);
}
#endif

void SDKStringConvert::to_buffer(const std::string& str, char* dst, size_t dstSizeInChars)
{
#ifdef _WIN32
	strcpy_s(dst, dstSizeInChars, str.c_str());
#else
	strncpy(dst, str.c_str(), dstSizeInChars);
    // If dst was too short it wasn't null-terminated
    dst[dstSizeInChars-1] = 0;
#endif
}

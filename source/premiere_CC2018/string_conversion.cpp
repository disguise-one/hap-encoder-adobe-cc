#include "string_conversion.hpp"

#ifndef _WIN64
#include <codecvt>
#include <CoreFoundation/CFString.h>
#include <vector>
#else
#include <Windows.h>
#endif

#include <locale>

#ifdef __APPLE__
template <class T>
static inline int aUTF16CharLength(const T inStr)
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

std::wstring SDKStringConvert::to_wstring(const A_UTF16Char *str)
{
#ifdef _WIN32
	return std::wstring(reinterpret_cast<const wchar_t*>(str));
#else
	return to_wstring(to_string(str));
#endif
}

std::wstring SDKStringConvert::to_wstring(const prUTF16Char* str)
{
#ifdef _WIN32
	return std::wstring(str);
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

std::string SDKStringConvert::to_string(const A_UTF16Char *str)
{
#ifdef _WIN32
    return to_string(to_wstring(str));
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

std::string SDKStringConvert::to_string(const prUTF16Char* str)
{
#ifdef _WIN32
	return to_string(to_wstring(str));
#else
	// TODO: CHECK
	CFStringRef input = CFStringCreateWithCharacters(NULL, str, aUTF16CharLength(str));
	const char* output = CFStringGetCStringPtr(input, kCFStringEncodingUTF8);
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

void SDKStringConvert::to_buffer(const std::string& str, prUTF16Char* dst, size_t dstChars)
{
	to_buffer(to_wstring(str), dst, dstChars);
}

void SDKStringConvert::to_buffer(const std::wstring& str, prUTF16Char* dst, size_t dstChars)
{
#ifdef __APPLE__
	// TODO: check
	int length = str.length();
	CFRange	range = { 0, kPrMaxPath };
	range.length = length;
	CFStringRef inputStringCFSR = CFStringCreateWithBytes(kCFAllocatorDefault,
		reinterpret_cast<const uint8_t*>(str.c_str()),
		length * sizeof(wchar_t),
		kCFStringEncodingUTF32LE,
		kPrFalse);
	CFStringGetBytes(inputStringCFSR,
		range,
		kCFStringEncodingUTF16,
		0,
		kPrFalse,
		reinterpret_cast<uint8_t*>(dst),
		dstChars * (sizeof(prUTF16Char)),
		NULL);
	dst[length] = 0;
	CFRelease(inputStringCFSR);
#elif defined _WIN32
	wcscpy_s(dst, dstChars, str.c_str());
#endif
}

void SDKStringConvert::to_buffer(const std::string& str, char* dst, size_t dstSizeInChars)
{
#ifdef _WIN32
	strcpy_s(dst, dstSizeInChars, str.c_str());
#else
	// TODO: make safe, will overflow atm
	strcpy(dst, str.c_str());
#endif
}

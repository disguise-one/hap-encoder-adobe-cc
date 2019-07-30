#pragma once

#include <string>

// Used by AE and Pr so re-typedef rather than include headers
#ifdef _WIN64
typedef wchar_t			prUTF16Char;
typedef unsigned short	A_u_short;
#else
typedef uint16_t        A_u_short;
typedef uint16_t		csSDK_uint16;
typedef csSDK_uint16	prUTF16Char;
#endif
typedef A_u_short       A_UTF16Char;


namespace SDKStringConvert {
	std::wstring to_wstring(const std::string& str);
	std::wstring to_wstring(const A_UTF16Char* str);
	std::wstring to_wstring(const prUTF16Char* str);
	std::string to_string(const std::wstring& fromUTF16);
	std::string to_string(const A_UTF16Char* str);
	std::string to_string(const prUTF16Char* str);
	void to_buffer(const std::string& str, prUTF16Char* dst, size_t dstSizeInChars);
	template <size_t size>
	void to_buffer(const std::string& str, prUTF16Char(&dst)[size]) {
		to_buffer(str, dst, size);
	}
	void to_buffer(const std::wstring& str, prUTF16Char* dst, size_t dstSizeInChars);
	template <size_t size>
	void to_buffer(const std::wstring& str, prUTF16Char(&dst)[size]) {
		to_buffer(str, dst, size);
	}
	void to_buffer(const std::string& str, char* dst, size_t dstSizeInChars);
	template <size_t size>
	void to_buffer(const std::string& str, char(&dst)[size]) {
		to_buffer(str, dst, size);
	}
}
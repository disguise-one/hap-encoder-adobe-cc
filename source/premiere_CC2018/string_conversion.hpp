#pragma once

#include <string>

// Used by AE and Pr so re-typedef rather than include headers
#ifdef _WIN64
typedef unsigned short  A_u_short;
#else
typedef uint16_t        A_u_short;
#endif
typedef A_u_short       A_UTF16Char;


// nuisance
std::wstring to_wstring(const std::string& str);
std::wstring to_wstring(const A_UTF16Char *str);
std::string to_string(const std::wstring& fromUTF16);
std::string to_string(const A_UTF16Char *str);
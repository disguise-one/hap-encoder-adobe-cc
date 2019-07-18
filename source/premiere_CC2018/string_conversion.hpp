#pragma once

#include <string>

// nuisance
std::wstring to_wstring(const std::string& str);
std::string to_string(const std::wstring& fromUTF16);
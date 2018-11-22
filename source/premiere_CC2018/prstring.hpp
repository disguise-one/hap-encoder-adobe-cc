#pragma once
#include <codecvt>
#include <vector>

#include <PrSDKStructs.h>

void copyConvertStringLiteralIntoUTF16(const wchar_t* inputString, prUTF16Char* destination);
void safeStrCpy(char *destStr, int size, const char *srcStr);

class StringForPr
{
public:
    StringForPr(const std::wstring &from)
        : prString_(from.size() + 1) {
        copyConvertStringLiteralIntoUTF16(from.c_str(), prString_.data());
    };
    const prUTF16Char *get() const {
        return prString_.data();
    }
private:
    std::vector<prUTF16Char> prString_;
};
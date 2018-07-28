#pragma once
#include <PrSDKStructs.h>

void copyConvertStringLiteralIntoUTF16(const wchar_t* inputString, prUTF16Char* destination);
void safeStrCpy(char *destStr, int size, const char *srcStr);

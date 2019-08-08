#pragma once
#include <vector>

#include <PrSDKStructs.h>
#include "string_conversion.hpp"

class StringForPr
{
public:
	StringForPr()
		: prString_(0) {
	};
    StringForPr(const std::wstring &from)
        : prString_(from.size() + 1) {
		SDKStringConvert::to_buffer(from, prString_.data(), prString_.size());
    };
	StringForPr(const std::string& from)
		: prString_(from.size() + 1) {
		SDKStringConvert::to_buffer(from, prString_.data(), prString_.size());
	};
	StringForPr& operator=(const std::wstring& from) {
		prString_.resize(from.size() + 1);
		SDKStringConvert::to_buffer(from, prString_.data(), prString_.size());
		return *this;
	};
	StringForPr& operator=(const std::string& from) {
		prString_.resize(from.size() + 1);
		SDKStringConvert::to_buffer(from, prString_.data(), prString_.size());
		return *this;
	};
	const prUTF16Char* get() const {
		return prString_.data();
	};
	operator const prUTF16Char* () const {
		return prString_.data();
	};
private:
    std::vector<prUTF16Char> prString_;
};

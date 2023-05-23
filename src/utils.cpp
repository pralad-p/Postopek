//
// Created by prlpr on 20/05/2023.
//

#include "utils.h"

std::string getCurrentTime() {
    std::chrono::system_clock::time_point p = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(p);
    char str[26];
    ctime_s(str, sizeof str, &t);
    return str;
}

[[maybe_unused]] std::wstring convertToWideString(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide_str = converter.from_bytes(str);
    return wide_str;
}
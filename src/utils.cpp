//
// Created by prlpr on 20/05/2023.
//

#include <string>
#include "utils.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

std::string getCurrentTime() {
    std::chrono::system_clock::time_point p = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(p);
    char str[26];
    ctime_s(str, sizeof str, &t);
    return str;
}

std::wstring convertToWideString(const std::string &str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring wide_str = converter.from_bytes(str);
    return wide_str;
}

std::string convertToHoursMinutes(const std::string &timeString) {
    std::string hhmm = timeString.substr(11, 5); // Extract HH:MM substring
    // Convert to 12-hour format
    int hour = std::stoi(hhmm.substr(0, 2));
    std::string amPm = (hour < 12) ? "AM" : "PM";
    if (hour == 0) {
        hour = 12; // Midnight is represented as 12:XX AM
    } else if (hour > 12) {
        hour %= 12; // Convert to 12-hour format
    }
    // Pad the hour with a leading zero if necessary
    std::string hourString = (hour < 10) ? "0" + std::to_string(hour) : std::to_string(hour);
    return hourString + hhmm.substr(2) + " " + amPm;
}

#pragma clang diagnostic pop
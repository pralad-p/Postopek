//
// Created by prlpr on 20/05/2023.
//

#include "Utilities.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

std::string getCurrentTime() {
    std::chrono::system_clock::time_point p = std::chrono::system_clock::now();
    time_t t = std::chrono::system_clock::to_time_t(p);
    char str[26];
    ctime_s(str, sizeof str, &t);
    return str;
}

std::string convertToStandardString(const std::wstring &wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
    return converter.to_bytes(wstr);
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

// trimStringInPlace from start
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));
}

// trimStringInPlace from end
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), s.end());
}

// trimStringInPlace from both ends
static inline void trimStringInPlace(std::string &s) {
    ltrim(s);
    rtrim(s);
}

std::vector<std::filesystem::path> checkTempFileAndGetFiles() {
    // Get the path to the TEMP folder
    auto tempFolderPath = std::filesystem::temp_directory_path();
    std::string config_file = "postopek_files.config";

    // Construct the full path to the file in the TEMP folder
    std::filesystem::path configFilePath = tempFolderPath / config_file;

    // Check if the file exists and it is a regular file
    if (!std::filesystem::is_regular_file(configFilePath)) {
        throw std::runtime_error("Not a valid file! Run first_run.bat first!");
    }

    std::ifstream file(configFilePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + configFilePath.string());
    }
    std::string line;
    while (std::getline(file, line)) {
        // Find the position of the '=' character
        size_t delimiterPos = line.find('=');
        if (delimiterPos != std::string::npos) {
            // Extract the key and value from the line
            std::string lineKey = line.substr(0, delimiterPos);
            std::string lineValue = line.substr(delimiterPos + 1);
            trimStringInPlace(lineKey);
            trimStringInPlace(lineValue);
            // Check if the extracted key matches the desired key
            if (lineKey == "MARKDOWN_FILES_DIR") {
                std::filesystem::path mdDirectoryPath = std::filesystem::path(lineValue);
                std::vector<std::filesystem::path> markdownFilePaths;

                for (const auto &entry: std::filesystem::directory_iterator(mdDirectoryPath)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".md") {
                        markdownFilePaths.push_back(entry.path());
                    }
                }
                if (markdownFilePaths.empty()) {
                    throw std::runtime_error("No markdown files in directory!");
                }
                return markdownFilePaths;
            }
        }
    }
    return std::vector<std::filesystem::path>{};
}

#pragma clang diagnostic pop
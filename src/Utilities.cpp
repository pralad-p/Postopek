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
void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !isspace(ch);
    }));
}

// trimStringInPlace from end
void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !isspace(ch);
    }).base(), s.end());
}

// trimStringInPlace from both ends
void trimStringInPlace(std::string &s) {
    ltrim(s);
    rtrim(s);
}

size_t findPathIndex(const std::vector<std::filesystem::path> &paths, const std::filesystem::path &target) {
    for (size_t i = 0; i < paths.size(); ++i) {
        if (std::filesystem::equivalent(paths[i], target)) {
            return i;
        }
    }
    return -1; // Return -1 if the target path is not found
}

std::pair<std::vector<std::filesystem::path>, std::deque<bool>> checkTempFileAndGetFiles() {
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
    std::vector<std::filesystem::path> markdownFilePaths;
    std::deque<bool> completelyNewFiles;
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
                for (const auto &entry: std::filesystem::directory_iterator(mdDirectoryPath)) {
                    if (entry.is_regular_file() && entry.path().extension() == ".md") {
                        markdownFilePaths.push_back(entry.path());
                    }
                }
                if (markdownFilePaths.empty()) {
                    throw std::runtime_error("No markdown files in directory!");
                } else {
                    completelyNewFiles.assign(markdownFilePaths.size(), true);
                }
            } else if (lineKey == "FILE") {
                auto visitedPath = std::filesystem::path(lineValue);
                auto index = findPathIndex(markdownFilePaths, visitedPath);
                completelyNewFiles[index] = false;
            }
        }
    }
    return std::make_pair(markdownFilePaths, completelyNewFiles);
}

std::string checkboxLabel(const std::string &label, bool checkbox_status) {
    static const std::regex timestamp_regex("\\[[0-1][0-9]:[0-5][0-9] (AM|PM)\\]");
    auto local_label = label;
    if (checkbox_status) {
        if (std::regex_search(local_label, timestamp_regex)) return local_label;
        std::string timeModded = "[" + convertToHoursMinutes(getCurrentTime()) + "]";
        local_label.insert(0, timeModded);
    } else {
        if (std::regex_search(local_label, timestamp_regex)) {
            local_label = std::regex_replace(local_label, timestamp_regex, "");
        }
    }
    return local_label;
}

ftxui::Element CheckboxDecorator(const std::shared_ptr<std::string> &label_ptr,
                                 const std::shared_ptr<bool> &checkbox_status_ptr,
                                 const std::shared_ptr<int> &iter_value_ptr,
                                 const ftxui::EntryState &state) {
    auto &label = *label_ptr;
    auto &checkbox_status = *checkbox_status_ptr;
    auto &iter_value = *iter_value_ptr;
    std::function < ftxui::Element(ftxui::Element) > base_style = state.state ? ftxui::inverted
                                                                              : ftxui::nothing;
    if (state.state) {
        base_style = base_style | color(ftxui::Color::Green);
    } else {
        base_style = base_style;
    }

    label = checkboxLabel(label, checkbox_status);
    return ftxui::hbox({
                               ftxui::text(std::to_wstring(iter_value) + L". [") | base_style,
                               ftxui::text(state.state ? L"✅" : L" ") | base_style,
                               ftxui::text(L"] ") | base_style,
                               ftxui::text(label) | base_style,
                       });
}

#pragma clang diagnostic pop
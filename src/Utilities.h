//
// Created by prlpr on 20/05/2023.
//

#ifndef POSTOPEK_UTILITIES_H
#define POSTOPEK_UTILITIES_H

#include <locale>
#include <utility>
#include <deque>
#include <chrono>
#include <ctime>
#include <codecvt>
#include <filesystem>
#include <string>
#include "ValidationCheck.h"

std::string getCurrentTime();

std::string convertToStandardString(const std::wstring &);

std::wstring convertToWideString(const std::string &);

std::string convertToHoursMinutes(const std::string &);

inline void ltrim(std::string &);

inline void rtrim(std::string &);

inline void trimStringInPlace(std::string &);

size_t findPathIndex(const std::vector<std::filesystem::path> &, const std::filesystem::path &);

std::pair<std::vector<std::filesystem::path>, std::deque<bool>> checkTempFileAndGetFiles();

#endif //POSTOPEK_UTILITIES_H

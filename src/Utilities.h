//
// Created by prlpr on 20/05/2023.
//

#ifndef POSTOPEK_UTILITIES_H
#define POSTOPEK_UTILITIES_H

#include <chrono>
#include <ctime>
#include <codecvt>
#include <filesystem>
#include <string>
#include "ValidationCheck.h"

std::string getCurrentTime();

std::wstring convertToWideString(const std::string &);

std::string convertToHoursMinutes(const std::string &);

static inline void ltrim(std::string &);

static inline void rtrim(std::string &);

static inline void trimStringInPlace(std::string &);

std::vector<std::filesystem::path> checkTempFileAndGetFiles();

#endif //POSTOPEK_UTILITIES_H

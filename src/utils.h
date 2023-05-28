//
// Created by prlpr on 20/05/2023.
//

#ifndef POSTOPEK_UTILS_H
#define POSTOPEK_UTILS_H

#include <chrono>
#include <ctime>
#include <codecvt>
#include <filesystem>
#include <string>
#include "validation.h"

std::string getCurrentTime();

std::wstring convertToWideString(const std::string &);

std::string convertToHoursMinutes(const std::string &);

static inline void ltrim(std::string &);

static inline void rtrim(std::string &);

static inline void trimStringInPlace(std::string &);

std::vector<std::filesystem::path> checkTempFileAndGetFiles();

#endif //POSTOPEK_UTILS_H

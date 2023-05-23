//
// Created by prlpr on 20/05/2023.
//

#ifndef POSTOPEK_UTILS_H
#define POSTOPEK_UTILS_H

#include <chrono>
#include <ctime>
#include <codecvt>

std::string getCurrentTime();
[[maybe_unused]] std::wstring convertToWideString(const std::string &);

std::string convertToHoursMinutes(const std::string &);

#endif //POSTOPEK_UTILS_H

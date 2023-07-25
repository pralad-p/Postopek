//
// Created by prlpr on 20/05/2023.
//

#ifndef POSTOPEK_UTILITIES_H
#define POSTOPEK_UTILITIES_H

#include <ftxui/dom/elements.hpp>  // for Element, hbox, text, inverted, color, nothing
#include <ftxui/screen/color.hpp>  // for Color::Green
#include "ftxui/component/component_options.hpp"
#include <string>                   // for std::string
#include <memory>                   // for std::shared_ptr
#include <functional>               // for std::function
#include <locale>
#include <utility>
#include <deque>
#include <chrono>
#include <ctime>
#include <codecvt>
#include <filesystem>
#include "ValidationCheck.h"

std::string getCurrentTime();

std::string convertToStandardString(const std::wstring &);

std::wstring convertToWideString(const std::string &);

std::string convertToHoursMinutes(const std::string &);

void ltrim(std::string &);

void rtrim(std::string &);

void trimStringInPlace(std::string &);

size_t findPathIndex(const std::vector<std::filesystem::path> &, const std::filesystem::path &);

std::pair<std::vector<std::filesystem::path>, std::deque<bool>> checkTempFileAndGetFiles();

std::string checkboxLabel(const std::string &, bool);

ftxui::Element CheckboxDecorator(const std::shared_ptr<std::string> &,
                                 const std::shared_ptr<bool> &,
                                 const std::shared_ptr<int> &,
                                 const ftxui::EntryState &);

#endif //POSTOPEK_UTILITIES_H

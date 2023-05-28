//
// Created by prlpr on 28/05/2023.
//

#ifndef POSTOPEK_VALIDATION_H
#define POSTOPEK_VALIDATION_H

#include <iostream>
#include <fstream>
#include <regex>
#include <string>

bool isValid(const std::string &line);

bool isValidMarkdownFile(const std::string &filename);

#endif //POSTOPEK_VALIDATION_H

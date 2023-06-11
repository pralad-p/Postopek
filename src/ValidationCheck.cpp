//
// Created by prlpr on 28/05/2023.
//

#include "ValidationCheck.h"

static bool saw_header;
static bool saw_list;

bool isValid(const std::string &line) {
    std::regex header("^# .+");
    std::regex list("^- .+");
    std::regex dropdown("^> .+");
    std::regex empty_line("^\\s*$");

    if (std::regex_match(line, empty_line)) {
        return true;
    } else if (std::regex_match(line, header)) {
        if (saw_header) return false;  // More than one header.
        saw_header = true;
        return true;
    } else if (std::regex_match(line, list)) {
        saw_list = true;
        return true;
    } else if (std::regex_match(line, dropdown)) {
        if (!saw_list) return false;  // Dropdown must be after a list item
        return true;
    }
    return false;
}

bool isValidMarkdownFile(const std::string &filename) {
    std::ifstream file(filename);
    saw_header = false;
    saw_list = false;

    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!isValid(line)) {
            return false;
        }
    }
    return true;
}

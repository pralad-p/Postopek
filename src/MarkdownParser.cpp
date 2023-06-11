//
// Created by prlpr on 29/05/2023.
//

#include <regex>
#include "MarkdownParser.h"

FileContainer::FileContainer(const std::filesystem::path &p) {
    file_path_ = p;
}

void FileContainer::Parse() {
    static const std::regex header("^# (.+)");
    static const std::regex list("^- (.+)");
    static const std::regex dropdown("^> (.+)");
    static const std::regex empty_line("^\\s*$");

    std::ifstream file(file_path_);
    if (!file) {
        throw std::runtime_error("Failed to open file! " + file_path_.string());
    }
    std::string line;
    std::smatch matches;
    std::string generic_comment;
    bool header_logged = false;
    while (std::getline(file, line)) {
        if (std::regex_match(line, empty_line)) {
            continue;
        }
        if (!header_logged && std::regex_search(line, matches, header)) {
            header_ = matches[1];
            header_logged = true;
            continue;
        }
        if (std::regex_search(line, matches, list)) {
            tasks_.push_back(matches[1]);
            comments_.push_back(generic_comment.empty() ? "" : generic_comment);
            generic_comment.clear();
            continue;
        }
        if (std::regex_search(line, matches, dropdown)) {
            if (!generic_comment.empty()) {
                generic_comment += "\n";
            }
            generic_comment += " => ";
            generic_comment += matches[1];
            generic_comment += "  ";
        }
    }
}

const std::string &FileContainer::getHeader() const {
    return header_;
}

const std::vector<std::string> &FileContainer::getTasks() const {
    return tasks_;
}

const std::vector<std::string> &FileContainer::getComments() const {
    return comments_;
}


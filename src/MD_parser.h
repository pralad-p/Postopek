//
// Created by prlpr on 29/05/2023.
//

#ifndef POSTOPEK_MD_PARSER_H
#define POSTOPEK_MD_PARSER_H

#include <string>
#include <vector>
#include <filesystem>
#include <fstream>


class FileContainer {
public:
    explicit FileContainer(const std::filesystem::path &);

    void Parse();

    const std::string &getHeader() const;

    void setHeader(const std::string &header);

    const std::vector<std::string> &getTasks() const;

    void setTasks(const std::vector<std::string> &tasks);

    const std::vector<std::string> &getComments() const;

    void setComments(const std::vector<std::string> &comments);

private:
    bool has_been_modified_;
    std::filesystem::path file_path_;
    std::string header_;
    std::vector<std::string> tasks_;
    std::vector<std::string> comments_;

};


#endif //POSTOPEK_MD_PARSER_H

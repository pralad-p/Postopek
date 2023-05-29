//
// Created by prlpr on 23/05/2023.
//

#ifndef POSTOPEK_PERSONALCOMPONENTS_H
#define POSTOPEK_PERSONALCOMPONENTS_H

#include "ftxui/component/component.hpp"
#include "MD_parser.h"
#include <string>


class TaskUI {
public:
    TaskUI();

    void loadMarkdownInfoToUIElements(const FileContainer &);

    std::shared_ptr<ftxui::ComponentBase> returnTaskInterface();

    void clearInterface();

private:
    std::vector<std::wstring> checkbox_labels_;
    std::vector<std::wstring> checkbox_comments_;
    std::vector<std::shared_ptr<bool>> checkbox_status_;
    std::vector<std::shared_ptr<bool>> checkbox_hovered_status_;
};


class UpdateButton {
public:
    UpdateButton(std::wstring &hover_text, std::string &input_value);

    ftxui::Component operator()();

private:
    std::wstring &hover_text_;
    std::string &input_value_;
    ftxui::Component button_;
};


#endif //POSTOPEK_PERSONALCOMPONENTS_H

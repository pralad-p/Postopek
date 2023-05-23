//
// Created by prlpr on 23/05/2023.
//

#ifndef POSTOPEK_UPDATEBUTTON_H
#define POSTOPEK_UPDATEBUTTON_H

#include "ftxui/component/component.hpp"
#include <string>

class UpdateButton {
public:
    UpdateButton(std::wstring &hover_text, std::string &input_value);

    ftxui::Component operator()();

private:
    std::wstring &hover_text_;
    std::string &input_value_;
    ftxui::Component button_;
};

#endif //POSTOPEK_UPDATEBUTTON_H

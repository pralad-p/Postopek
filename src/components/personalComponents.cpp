//
// Created by prlpr on 23/05/2023.
//

#include "personalComponents.h"
#include "utils.h"

UpdateButton::UpdateButton(std::wstring &hover_text, std::string &input_value)
        : hover_text_(hover_text), input_value_(input_value) {

    auto update_hover_text = [this] {
        std::wstring input_value_wstring(input_value_.begin(), input_value_.end());
        // Append the converted input_value to hover_text on a new line
        if (!hover_text_.empty()) {
            hover_text_ += L"\n";
        }
        hover_text_ += L"- [" + convertToWideString(convertToHoursMinutes(getCurrentTime())) + L"] ";
        hover_text_ += input_value_wstring;
        input_value_.clear();
    };

    button_ = ftxui::Button("Update", update_hover_text);
}

ftxui::Component UpdateButton::operator()() {
    return button_;
}
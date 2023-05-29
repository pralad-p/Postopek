//
// Created by prlpr on 23/05/2023.
//

#include <sstream>
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

TaskUI::TaskUI() {}

void TaskUI::loadMarkdownInfoToUIElements(const FileContainer &fCont) {
    auto currentContainerSize = fCont.getTasks().size();
    for (size_t i = 0; i < currentContainerSize; i++) {
        auto false_ptr = std::make_shared<bool>(false);
        checkbox_status_.push_back(false_ptr);
        checkbox_hovered_status_.push_back(false_ptr);
        checkbox_labels_.push_back(convertToWideString(fCont.getTasks().at(i)));
        checkbox_comments_.push_back(convertToWideString(fCont.getComments().at(i)));
    }
}

void TaskUI::clearInterface() {
    checkbox_status_.clear();
    checkbox_hovered_status_.clear();
    checkbox_labels_.clear();
    checkbox_comments_.clear();
}


std::shared_ptr<ftxui::ComponentBase> TaskUI::returnTaskInterface() {
    ftxui::Components checkboxes, hoverable_checkboxes, hovering_renderers;
    size_t i = 0;
    auto taskInterface = ftxui::Container::Vertical({});
    for (i = 0; i < checkbox_labels_.size(); i++) {
        auto checkbox_option = ftxui::CheckboxOption();
        auto checkbox_status = checkbox_status_.at(i).get();
        auto label = checkbox_labels_.at(i);
        auto iter_value = i;
        auto checkboxLabel = [&label, &checkbox_status]() -> std::wstring {
            if (checkbox_status) {
                std::wstring timeModded = L"[" + convertToWideString(convertToHoursMinutes(getCurrentTime())) + L"] ";
                label.insert(0, timeModded);
            }
            return label;
        };
        auto checkbox_decorator = [&iter_value, &checkboxLabel](const ftxui::EntryState &state) {
            std::function < ftxui::Element(ftxui::Element) > base_style = state.state ? ftxui::inverted
                                                                                      : ftxui::nothing;
            if (state.state) {
                base_style = base_style | color(ftxui::Color::Green);
            } else {
                base_style = base_style;
            }

            return ftxui::hbox({
                                       ftxui::text(std::to_wstring(iter_value) + L". [") | base_style,
                                       ftxui::text(state.state ? L"✅" : L" ") | base_style,
                                       ftxui::text(L"] ") | base_style,
                                       ftxui::text(checkboxLabel()) | base_style,
                               });
        };
        checkbox_option.transform = checkbox_decorator;
        auto cb = ftxui::Checkbox(&checkbox_labels_.at(i), checkbox_status_.at(i).get(), checkbox_option);
        checkboxes.push_back(cb);
    }
    for (i = 0; i < checkboxes.size(); i++) {
        auto hoverable_cb = Hoverable(checkboxes.at(i),
                                      [&, i]() { *checkbox_hovered_status_.at(i) = true; },
                                      [&, i]() { *checkbox_hovered_status_.at(i) = false; });
        hoverable_checkboxes.push_back(hoverable_cb);
    }
    for (i = 0; i < checkboxes.size(); i++) {
        std::wstring hover_text = checkbox_comments_.at(i);
        auto hover_text_renderer = ftxui::Renderer([&] {
            if (checkbox_hovered_status_.at(i)) {
                // Convert hover_text from std::wstring to std::string.
                std::string hover_text_str = std::string(hover_text.begin(), hover_text.end());
                std::vector<std::string> lines;
                std::istringstream iss(hover_text_str);
                for (std::string line; std::getline(iss, line);) {
                    lines.push_back(line);
                }
                std::vector<ftxui::Element> elements;
                elements.reserve(lines.size());
                for (const auto &line: lines) {
                    elements.push_back(ftxui::text(line) | color(ftxui::Color::Red) | ftxui::bold);
                }
                return ftxui::vbox(elements) | ftxui::center;
            } else {
                return nothing(ftxui::text(""));
            }
        });
        hovering_renderers.push_back(hover_text_renderer);
    }
    for (i = 0; i < checkboxes.size(); i++) {
        taskInterface->Add(hoverable_checkboxes.at(i));
        taskInterface->Add(hovering_renderers.at(i));
    }
    taskInterface |= ftxui::vscroll_indicator;
    return taskInterface;
}

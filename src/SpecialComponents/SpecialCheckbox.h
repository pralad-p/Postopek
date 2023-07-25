//
// Created by prlpr on 25/07/2023.
//

#ifndef POSTOPEK_SPECIALCHECKBOX_H
#define POSTOPEK_SPECIALCHECKBOX_H

#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/component/component_options.hpp"
#include "Utilities.h"
#include <sstream>
#include <filesystem>
#include <algorithm>
#include <utility>

class SpecialCheckbox : public ftxui::ComponentBase {
public:
    SpecialCheckbox(ftxui::Component checkbox, std::shared_ptr<bool> state, std::shared_ptr<std::string> label)
            : state_(std::move(state)), checkbox_(std::move(checkbox)), label_(std::move(label)) {
    }

    ftxui::Element Render() override;

    bool OnEvent(ftxui::Event event) override;

    bool OnMouseEvent(ftxui::Event event);

private:
    ftxui::Component checkbox_;
    std::shared_ptr<bool> state_;
    std::shared_ptr<std::string> label_;
    bool hovered_ = false;
    ftxui::Box box_;
};

#endif //POSTOPEK_SPECIALCHECKBOX_H

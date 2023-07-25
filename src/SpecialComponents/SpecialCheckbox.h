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
    SpecialCheckbox(const std::string *label, bool *state, ftxui::CheckboxOption option)
            : checkbox_(ftxui::Checkbox(label, state, option)) {
    }

    ftxui::Element Render() override;

    bool OnEvent(ftxui::Event event) override;

private:
    ftxui::Component checkbox_;

};

#endif //POSTOPEK_SPECIALCHECKBOX_H
